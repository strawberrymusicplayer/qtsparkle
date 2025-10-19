/* This file is part of qtsparkle.
   Copyright (c) 2010 David Sansome <me@davidsansome.com>

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

#include "common.h"
#include "uicontroller.h"
#include "updatechecker.h"
#include "updater.h"

#include <QCoreApplication>
#include <QIcon>
#include <QMessageBox>
#include <QPointer>
#include <QPushButton>
#include <QTimer>
#include <QTranslator>
#include <QUrl>
#include <QtDebug>
#include <QLocale>


inline static void InitTranslationsResource() {
  Q_INIT_RESOURCE(qtsparkle_translations);
}


namespace qtsparkle {

void LoadTranslations(const QString& language) {
  static bool sLoadedTranslations = false;
  if (sLoadedTranslations) {
    return;
  }
  sLoadedTranslations = true;
  
  InitTranslationsResource();
  
  QTranslator* t = new QTranslator;
  if (t->load(language, ":/qtsparkle/translations/")) {
    QCoreApplication::installTranslator(t);
  } else {
    delete t;
  }
}

struct Updater::Private {
  Private(const QUrl& appcast_url, QWidget* parent_widget, Updater* updater)
    : parent_widget_(parent_widget),
      updater_(updater),
      network_(NULL),
      appcast_url_(appcast_url),
      update_check_timer_(NULL),
      update_check_interval_msec_(86400000) // one day
  {
  }

  void CheckNow();

  QWidget* parent_widget_;
  Updater* updater_;

  QNetworkAccessManager* network_;
  QIcon icon_;
  QString version_;

  QUrl appcast_url_;

  QEvent::Type ask_permission_event_;
  QEvent::Type auto_check_event_;

  QTimer* update_check_timer_;
  int update_check_interval_msec_;

  QPointer<UiController> controller_;
};


Updater::Updater(const QUrl& appcast_url, QWidget* parent)
  : QObject(parent),
    d(new Private(appcast_url, parent, this))
{
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
  connect(d->update_check_timer_, SIGNAL(timeout()), SLOT(AutoCheck()));
}

Updater::~Updater() {
}

void Updater::SetIcon(const QIcon& icon) {
  d->icon_ = icon;
}

void Updater::SetNetworkAccessManager(QNetworkAccessManager* network) {
  d->network_ = network;
}

void Updater::SetVersion(const QString& version) {
  d->version_ = version;
}

void Updater::SetUpdateInterval(int msec) {
  if (msec < 3600000)
    return;

  d->update_check_interval_msec_ = msec;
}

bool Updater::event(QEvent* e) {
  if (e->type() == d->auto_check_event_) {
    d->CheckNow();
    return true;
  }

  return QObject::event(e);
}

void Updater::CheckNow() {
  d->CheckNow();
}

void Updater::AutoCheck() {
  d->CheckNow();
}

void Updater::Private::CheckNow() {
  delete controller_;
  controller_ = new UiController(updater_, parent_widget_);
  controller_->SetNetworkAccessManager(network_);
  controller_->SetIcon(icon_);
  controller_->SetVersion(version_);

  UpdateChecker* checker = new UpdateChecker(updater_);
  checker->SetNetworkAccessManager(network_);
  checker->SetVersion(version_);

  connect(checker, SIGNAL(CheckStarted()), controller_, SLOT(CheckStarted()));
  connect(checker, SIGNAL(CheckFailed(QString)), controller_, SLOT(CheckFailed(QString)));
  connect(checker, SIGNAL(UpdateAvailable(AppCastPtr)), controller_, SLOT(UpdateAvailable(AppCastPtr)));
  connect(checker, SIGNAL(UpToDate()), controller_, SLOT(UpToDate()));

  checker->Check(appcast_url_);

  // The UiController will delete itself when the check is finished.

  update_check_timer_->start(update_check_interval_msec_);

}

} // namespace qtsparkle
