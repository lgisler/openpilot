#include "selfdrive/ui/soundd/sound.h"

#include "cereal/messaging/messaging.h"
#include "selfdrive/common/util.h"

// TODO: detect when we can't play sounds
// TODO: detect when we can't display the UI

Sound::Sound(QObject *parent) : sm({"carState", "controlsState"}) {
  const QString sound_asset_path = Hardware::TICI() ? "../../assets/sounds_tici/" : "../../assets/sounds/";
  for (auto &[alert, fn, loops] : sound_list) {
    QSoundEffect *s = new QSoundEffect(this);
    QObject::connect(s, &QSoundEffect::statusChanged, [=]() {
      assert(s->status() != QSoundEffect::Error);
    });
    s->setVolume(Hardware::MIN_VOLUME);
    s->setSource(QUrl::fromLocalFile(sound_asset_path + fn));
    sounds[alert] = {s, loops};
  }

  QTimer *timer = new QTimer(this);
  QObject::connect(timer, &QTimer::timeout, this, &Sound::update);
  timer->start(1000 / UI_FREQ);
};

void Sound::update() {
  sm.update(0);

  // scale volume with speed
  if (sm.updated("carState")) {
    float volume = util::map_val(sm["carState"].getCarState().getVEgo(), 0.f, 20.f,
                                 Hardware::MIN_VOLUME, Hardware::MAX_VOLUME);
    for (auto &[s, loops] : sounds) {
      s->setVolume(std::round(100 * volume) / 100);
    }
  }

  setAlert(Alert::get(sm, 1));
}

void Sound::setAlert(const Alert &alert) {
  if (!current_alert.equal(alert)) {
    current_alert = alert;
    // stop sounds
    for (auto &[s, loops] : sounds) {
      // Only stop repeating sounds
      if (s->loopsRemaining() > 1 || s->loopsRemaining() == QSoundEffect::Infinite) {
        s->stop();
      }
    }

    // play sound
    if (alert.sound != AudibleAlert::NONE) {
      auto &[s, loops] = sounds[alert.sound];
      s->setLoopCount(loops);
      s->play();
    }
  }
}
