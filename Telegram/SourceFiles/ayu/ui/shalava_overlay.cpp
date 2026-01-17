#include "ayu/ui/shalava_overlay.h"
#include "ayu/ayu_settings.h"
#include <QtGui/QPainter>
#include <QtGui/QResizeEvent>
#include <QtCore/QRandomGenerator>
#include <QtGui/QPainterPath>
#include <QtCore/QTimer>

namespace Ayu {
namespace Ui {

namespace {
    float randomFloat(float min, float max) {
        return min + QRandomGenerator::global()->generateDouble() * (max - min);
    }
}

ShalavaOverlay::ShalavaOverlay(QWidget *parent) : QWidget(parent) {
    setAttribute(Qt::WA_TransparentForMouseEvents, true);
    setAttribute(Qt::WA_NoSystemBackground);
    
    _timer.setInterval(16); // ~60 FPS
    connect(&_timer, &QTimer::timeout, this, [=] {
        updateParticles();
        update();
    });
    
    // Load icons
    _starIcon.load(":/gui/art/ayu/shalava/star.svg");
    _checkmarkIcon.load(":/gui/art/ayu/shalava/checkmark.svg");
    _uzbekchanIcon.load(":/gui/art/ayu/shalava/uzbekchan.svg");
    _ghostIcon.load(":/gui/art/ayu/shalava/ghost.svg");

    updateSettings();

    // Subscribe to settings changes
    AyuSettings::get_shalavaModeReactive().start(
        [=](int mode) {
            updateSettings(); // Refresh all settings including mode
        },
        [](const auto &) {},
        [] {},
        lifetime()
    );
}

void ShalavaOverlay::updateSettings() {
    auto &settings = AyuSettings::getInstance();
    int newMode = settings.shalavaMode;
    
    // Check if other settings changed that might require updates
    _safeMode = settings.shalavaSafeMode;
    _particleLimit = settings.shalavaParticleLimit;
    _textOverlay = settings.shalavaTextOverlay;
    _captureMouse = settings.shalavaOverlayCapturesMouse;
    
    setAttribute(Qt::WA_TransparentForMouseEvents, !_captureMouse);
    
    if (newMode != _mode) {
        setMode(newMode);
    } else {
        // If mode is same but other settings changed, ensure timer state
        if (_mode > 0 && !_timer.isActive()) {
            _timer.start();
        }
    }
}

void ShalavaOverlay::setMode(int mode) {
    _mode = mode;
    if (_mode == 0) {
            _particles.clear();
            _timer.stop();
            update();
    } else {
            if (!_timer.isActive()) _timer.start();
    }
}

void ShalavaOverlay::updateParticles() {
    if (width() <= 0 || height() <= 0) return;

    // Mode specific logic
    int targetParticles = 0;
    float speedMult = 1.0f;
    
    if (_mode == 1) { // SHALAVA
        targetParticles = std::min(40, _particleLimit);
        speedMult = 0.5f;
    } else if (_mode == 2) { // SUPER
        targetParticles = std::min(120, _particleLimit);
        speedMult = 1.0f;
    } else if (_mode == 3) { // ULTRA
        targetParticles = _particleLimit;
        speedMult = _safeMode ? 1.0f : 2.0f;
    }

    if (_safeMode) {
        targetParticles /= 2;
        speedMult *= 0.5f;
    }

    // Spawn
    if (_particles.size() < targetParticles) {
        // Higher chance to spawn if far from target
        if (randomFloat(0, 1) < 0.2 * speedMult) spawnParticle();
    }

    // Shake
    if (_mode > 0) {
        float shakeAmp = 0;
        if (_mode == 1) shakeAmp = 2;
        else if (_mode == 2) shakeAmp = 6;
        else if (_mode == 3) shakeAmp = _safeMode ? 10 : 25;

        _shakeX = (int)randomFloat(-shakeAmp, shakeAmp);
        _shakeY = (int)randomFloat(-shakeAmp, shakeAmp);
    } else {
        _shakeX = 0; _shakeY = 0;
    }

    // Move
    for (auto &p : _particles) {
        p.x += p.vx * speedMult;
        p.y += p.vy * speedMult;
        p.rotation += p.vrotation * speedMult;
        p.life -= 0.005f * speedMult;
        
        if (p.x < -100 || p.x > width() + 100 || p.y < -100 || p.y > height() + 100) {
            p.life = 0; // kill
        }
    }
    
    // Remove dead
    _particles.erase(std::remove_if(_particles.begin(), _particles.end(), [](const Particle &p) {
        return p.life <= 0;
    }), _particles.end());
}

void ShalavaOverlay::spawnParticle() {
    if (width() <= 0 || height() <= 0) return;

    Particle p;
    p.x = randomFloat(0, width());
    p.y = randomFloat(0, height());
    p.vx = randomFloat(-2, 2);
    p.vy = randomFloat(-2, 2);
    p.rotation = randomFloat(0, 360);
    p.vrotation = randomFloat(-5, 5);
    p.life = 1.0f;
    
    // Types
    // 0: Star, 1: Checkmark, 2: Uzbekchan, 3: Text
    float r = randomFloat(0, 1);
    
    if (_mode == 1) {
        p.type = 0; // mostly stars
        p.size = randomFloat(10, 20);
    } else if (_mode == 2) {
         if (r < 0.6) p.type = 0; // star
         else if (r < 0.9) p.type = 1; // checkmark
         else p.type = (_textOverlay ? 3 : 0); // text
         p.size = randomFloat(15, 30);
    } else { // ULTRA
         if (r < 0.4) p.type = 0;
         else if (r < 0.7) p.type = 1;
         else if (r < 0.85) p.type = 2; // uzbekchan
         else p.type = (_textOverlay ? 3 : 0);
         p.size = randomFloat(20, 50);
    }
    
    if (p.type == 3) {
        if (_mode == 3 && randomFloat(0,1) > 0.5) p.text = "pashol naxxuy";
        else p.text = "uzbekgram";
        
        // Text moves faster
        p.vx *= 2;
    }
    
    _particles.push_back(p);
}

void ShalavaOverlay::paintEvent(QPaintEvent *e) {
    if (_mode == 0) return;
    if (!isVisible()) return;

    QPainter p(this);
    if (!p.isActive()) return;

    p.setRenderHint(QPainter::Antialiasing);
    p.setRenderHint(QPainter::SmoothPixmapTransform);
    
    // Apply shake
    p.translate(_shakeX, _shakeY);

    for (const auto &part : _particles) {
        p.save();
        p.translate(part.x, part.y);
        p.rotate(part.rotation);
        p.setOpacity(part.life);
        
        float s = part.size;
        
        switch(part.type) {
            case 0: // Star
                if (!_starIcon.isNull()) p.drawPixmap(-s/2, -s/2, s, s, _starIcon);
                break;
            case 1: // Checkmark
                if (!_checkmarkIcon.isNull()) p.drawPixmap(-s/2, -s/2, s, s, _checkmarkIcon);
                break;
            case 2: // Uzbekchan
                if (!_uzbekchanIcon.isNull()) p.drawPixmap(-s/2, -s/2, s, s, _uzbekchanIcon);
                break;
            case 3: // Text
                p.setPen(QPen(Qt::red)); // bold red text
                QFont f = p.font();
                f.setBold(true);
                if (s > 0) {
                    f.setPixelSize(s);
                    p.setFont(f);
                    p.drawText(QRectF(-s*5, -s, s*10, s*2), Qt::AlignCenter, part.text);
                }
                break;
        }
        
        p.restore();
    }
}

void ShalavaOverlay::resizeEvent(QResizeEvent *e) {
    QWidget::resizeEvent(e);
}

bool ShalavaOverlay::event(QEvent *e) {
    return QWidget::event(e);
}

} // Ui
} // Ayu