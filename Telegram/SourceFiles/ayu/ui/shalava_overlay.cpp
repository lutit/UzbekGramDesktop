#include "ayu/ui/shalava_overlay.h"
#include "ayu/ayu_settings.h"
#include <QtGui/QPainter>
#include <QtGui/QResizeEvent>
#include <QtCore/QRandomGenerator>
#include <QtGui/QPainterPath>
#include <QtCore/QTimer>
#include <cmath>

namespace Ayu {
namespace Ui {

namespace {
    float randomFloat(float min, float max) {
        return min + QRandomGenerator::global()->generateDouble() * (max - min);
    }

    float safeRandomFloat(float min, float max) {
        const auto v = randomFloat(min, max);
        if (std::isnan(v) || std::isinf(v)) return (min + max) * 0.5f;
        return v;
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
        _lifetime
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

    // Safety check for runaway particles (hard cap)
    static constexpr int kHardCap = 500;
    if (_particles.size() > kHardCap) {
        _particles.clear();
        return;
    }

    // Mode specific logic
    int targetParticles = 0;
    float speedMult = 1.0f;
    
    // Hard cap to prevent crashes
    int safeLimit = std::clamp(_particleLimit, 10, 150);

    if (_mode == 1) { // SHALAVA
        targetParticles = std::min(40, safeLimit);
        speedMult = 0.5f;
    } else if (_mode == 2) { // SUPER
        targetParticles = std::min(90, safeLimit);
        speedMult = 0.9f;
    } else if (_mode == 3) { // ULTRA
        targetParticles = std::min(safeLimit, 120);
        speedMult = _safeMode ? 1.0f : 1.2f;
    }

    if (_safeMode) {
        targetParticles /= 2;
        speedMult *= 0.5f;
    }

    // Spawn
    if (_particles.size() < targetParticles) {
        // Higher chance to spawn if far from target
        if (safeRandomFloat(0, 1) < 0.2 * speedMult) spawnParticle();
    }

    // Shake
    if (_mode > 0) {
        float shakeAmp = 0;
        if (_mode == 1) shakeAmp = 2;
        else if (_mode == 2) shakeAmp = 6;
        else if (_mode == 3) shakeAmp = _safeMode ? 10 : 18;

        _shakeX = (int)safeRandomFloat(-shakeAmp, shakeAmp);
        _shakeY = (int)safeRandomFloat(-shakeAmp, shakeAmp);
    } else {
        _shakeX = 0; _shakeY = 0;
    }

    // Move
    for (auto &p : _particles) {
        p.x += p.vx * speedMult;
        p.y += p.vy * speedMult;
        p.rotation += p.vrotation * speedMult;
        p.life -= 0.005f * speedMult;
        
        if (std::isnan(p.x) || std::isnan(p.y) || std::isnan(p.life)) {
            p.life = 0;
        }

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
    p.x = safeRandomFloat(0, width());
    p.y = safeRandomFloat(0, height());
    p.vx = safeRandomFloat(-2, 2);
    p.vy = safeRandomFloat(-2, 2);
    p.rotation = safeRandomFloat(0, 360);
    p.vrotation = safeRandomFloat(-5, 5);
    p.life = 1.0f;
    
    // Types
    // 0: Star, 1: Checkmark, 2: Uzbekchan, 3: Text
    float r = randomFloat(0, 1);
    
    if (_mode == 1) {
        p.type = 0; // mostly stars
        p.size = randomFloat(10, 18);
    } else if (_mode == 2) {
         if (r < 0.6) p.type = 0; // star
         else if (r < 0.9) p.type = 1; // checkmark
         else p.type = (_textOverlay ? 3 : 0); // text
         p.size = randomFloat(14, 26);
    } else { // ULTRA
         if (r < 0.7) p.type = 4; // Premium Star
         else if (r < 0.85) p.type = 2; // uzbekchan
         else p.type = (_textOverlay ? 3 : 4);
         p.size = randomFloat(20, 45);
    }
    
    if (p.type == 3) {
        if (_mode == 3 && randomFloat(0,1) > 0.5) p.text = "pashol naxxuy";
        else p.text = "uzbekgram";
        
        // Text moves faster but clamp to avoid runaway
        p.vx = std::clamp(p.vx * 1.5f, -4.f, 4.f);
        p.vy = std::clamp(p.vy * 1.5f, -4.f, 4.f);
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
                {
                    p.setPen(Qt::red); // bold red text (removed QPen alloc)
                    QFont f = p.font();
                    f.setBold(true);
                    if (s > 0) {
                        f.setPixelSize(s);
                        p.setFont(f);
                        p.drawText(QRectF(-s*5, -s, s*10, s*2), Qt::AlignCenter, part.text);
                    }
                }
                break;
            case 4: // Premium Star
                {
                    // Premium gradient
                    QLinearGradient gradient(-s/2, -s/2, s/2, s/2);
                    gradient.setColorAt(0, QColor(255, 215, 0));   // Gold
                    gradient.setColorAt(0.5, QColor(255, 165, 0)); // Orange
                    gradient.setColorAt(1, QColor(255, 69, 0));    // Red-Orange

                    QPainterPath starPath;
                    const int points = 5;
                    const double outerR = s / 2.0;
                    const double innerR = outerR * 0.4;

                    for (int i = 0; i < points * 2; ++i) {
                        const double r = (i % 2 == 0) ? outerR : innerR;
                        const double angle = M_PI / 2 + i * M_PI / points;
                        const double x = r * std::cos(angle);
                        const double y = -r * std::sin(angle);
                        if (i == 0) starPath.moveTo(x, y);
                        else starPath.lineTo(x, y);
                    }
                    starPath.closeSubpath();

                    p.setBrush(gradient);
                    p.setPen(Qt::NoPen);
                    p.drawPath(starPath);
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
