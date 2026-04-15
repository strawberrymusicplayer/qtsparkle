/* This file is part of qtsparkle.
   Copyright (c) 2010 David Sansome <me@davidsansome.com>
   Copyright 2018-2026, Jonas Kvinge <jonas@jkvinge.net>

   Permission is hereby granted, free of charge, to any person obtaining a copy
   of this software and associated documentation files (the "Software"), to deal
   in the Software without restriction, including without limitation the rights
   to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
   copies of the Software, and to permit persons to whom the Software is
   furnished to do so, subject to the following conditions:

   The above copyright notice and this permission notice shall be included in
   all copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
   AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
   OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
   THE SOFTWARE.
*/

#include <QCoreApplication>
#include <QString>
#include <QUrl>
#include <QIcon>
#include <QPointer>
#include <QTimer>
#include <QLocale>
#include <QTranslator>
#include <QMessageBox>
#include <QPushButton>

#include "common.h"
#include "uicontroller.h"
#include "updatechecker.h"
#include "updater.h"

using namespace Qt::Literals::StringLiterals;

inline static void InitTranslationsResource() {
  Q_INIT_RESOURCE(qtsparkle_translations);
}


namespace qtsparkle {

bool LoadTranslations(const QString &language) {

  static bool sTranslationsResourceInit = false;
  static bool sTranslationsLoaded = false;

  if (!sTranslationsResourceInit) {
    InitTranslationsResource();
    sTranslationsResourceInit = true;
  }

  if (sTranslationsLoaded) {
    return false;
  }

  QTranslator *t = new QTranslator;
  if (t->load(language, ":/qtsparkle/translations/"_L1)) {
    QCoreApplication::installTranslator(t);
    sTranslationsLoaded = true;
    return true;
  }

  delete t;
  return false;

}

struct Updater::Private {
  Private(const QUrl &appcast_url, QWidget *parent_widget, Updater *updater)
      : parent_widget_(parent_widget),
        updater_(updater),
        network_(nullptr),
        appcast_url_(appcast_url),
        update_check_timer_(nullptr),
        update_check_interval_msec_(86400000)  // one day
  {}

  void CheckNow(const bool quiet);

  QWidget *parent_widget_;
  Updater *updater_;

  QNetworkAccessManager *network_;
  QIcon icon_;
  QString version_;

  QUrl appcast_url_;

  QEvent::Type ask_permission_event_;
  QEvent::Type auto_check_event_;

  QTimer *update_check_timer_;
  int update_check_interval_msec_;

  QPointer<UiController> controller_;
};


Updater::Updater(const QUrl &appcast_url, QWidget *parent)
    : QObject(parent),
      d(new Private(appcast_url, parent, this)) {

  // Load translations if they haven't been loaded already.
  LoadTranslations(QLocale::system().name());

  d->ask_permission_event_ = QEvent::Type(QEvent::registerEventType());
  d->auto_check_event_ = QEvent::Type(QEvent::registerEventType());
  d->version_ = QCoreApplication::applicationVersion();

  if (parent) {
    SetIcon(parent->windowIcon());
  }

  QCoreApplication::postEvent(this, new QEvent(d->auto_check_event_));

  d->update_check_timer_ = new QTimer(this);
  d->update_check_timer_->setSingleShot(true);
  QObject::connect(d->update_check_timer_, &QTimer::timeout, this, &Updater::AutoCheck);

}

Updater::~Updater() = default;

void Updater::SetIcon(const QIcon &icon) {
  d->icon_ = icon;
}

void Updater::SetNetworkAccessManager(QNetworkAccessManager *network) {
  d->network_ = network;
}

void Updater::SetVersion(const QString &version) {
  d->version_ = version;
}

void Updater::SetUpdateInterval(const int msec) {

  if (msec < 3600000) {
    return;
  }

  d->update_check_interval_msec_ = msec;

}

bool Updater::event(QEvent *e) {

  if (e->type() == d->auto_check_event_) {
    d->CheckNow(true);
    return true;
  }

  return QObject::event(e);

}

void Updater::CheckNow() {
  d->CheckNow(false);
}

void Updater::AutoCheck() {
  d->CheckNow(true);
}

void Updater::Private::CheckNow(const bool quiet) {

  delete controller_;
  controller_ = new UiController(quiet, updater_, parent_widget_);
  controller_->SetNetworkAccessManager(network_);
  controller_->SetIcon(icon_);
  controller_->SetVersion(version_);

  UpdateChecker *checker = new UpdateChecker(updater_);
  checker->SetNetworkAccessManager(network_);
  checker->SetVersion(version_);

  QObject::connect(checker, &UpdateChecker::CheckStarted, controller_, &UiController::CheckStarted);
  QObject::connect(checker, &UpdateChecker::CheckFailed, controller_, &UiController::CheckFailed);
  QObject::connect(checker, &UpdateChecker::UpdateAvailable, controller_, &UiController::UpdateAvailable);
  QObject::connect(checker, &UpdateChecker::UpToDate, controller_, &UiController::UpToDate);

  checker->Check(appcast_url_, !quiet);

  // The UiController will delete itself when the check is finished.

  update_check_timer_->start(update_check_interval_msec_);

}

}  // namespace qtsparkle
