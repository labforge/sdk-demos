#ifndef __CAMERAVIEW_HPP__
#define __CAMERAVIEW_HPP__

#include <memory>
#include <QtWidgets/QLabel>
#include <QtWidgets/QRubberBand>
#include <QMouseEvent>

namespace labforge::ui {

class CameraView : public QLabel {
  Q_OBJECT
public:
  explicit CameraView(QWidget *parent = nullptr);
  virtual ~CameraView() {};

  void resizeEvent(QResizeEvent* event) override;
  void mousePressEvent(QMouseEvent* event) override;
  void mouseReleaseEvent(QMouseEvent* event) override;
  void mouseMoveEvent(QMouseEvent* event) override;
  void setImage(QImage &img, bool redraw);
  void addTarget(QRect &pos, QString &label, QColor &color, int width);
  void addFeature(QPoint &pos, QColor &color, int width);
  void reset();
  void redrawPixmap();

private:
  QPoint m_origin;
  QRect m_crop;
  std::unique_ptr<QRubberBand> m_rubberband;
  std::unique_ptr<QPixmap> m_last_frame;
  bool m_scaled;

};

} // namespace labforge::ui

#endif // #define __CAMERAVIEW_HPP__