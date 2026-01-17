#pragma once

#include <QtWidgets/QWidget>
#include <QtCore/QTimer>
#include <QtGui/QPixmap>
#include <vector>

namespace Ayu {
namespace Ui {

class ShalavaOverlay : public QWidget {
public:
	explicit ShalavaOverlay(QWidget *parent = nullptr);

	void setMode(int mode);
	void updateSettings();

protected:
	void paintEvent(QPaintEvent *e) override;
	void resizeEvent(QResizeEvent *e) override;
	bool event(QEvent *e) override;

private:
	struct Particle {
		float x, y;
		float vx, vy;
		float size;
		float rotation;
		float vrotation;
		int type; // 0: Star, 1: Checkmark, 2: Uzbekchan, 3: Text
		QString text;
		float alpha;
		float life; // 0.0 to 1.0
	};

	void updateParticles();
	void spawnParticle();
	void spawnText();
	void reset();

	int _mode = 0; // 0=OFF, 1=SHALAVA, 2=SUPER, 3=ULTRA
	bool _safeMode = false;
	int _particleLimit = 100;
	bool _textOverlay = true;
	bool _captureMouse = false;

	std::vector<Particle> _particles;
	QTimer _timer;

	QPixmap _ghostIcon;
	QPixmap _starIcon;
	QPixmap _checkmarkIcon;
	QPixmap _uzbekchanIcon;
	
	int _shakeX = 0;
	int _shakeY = 0;
};

} // namespace Ui
} // namespace Ayu
