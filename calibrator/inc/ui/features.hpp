#ifndef _features_hpp_
#define _features_hpp_

#include <qwt_plot.h>
#include <qwt_plot_curve.h>
#include <deque>
#include <memory>

#define DEFAULT_SAMPLES_SHOWN 30

// QwT rundown here: https://ghorwin.github.io/qwtbook

namespace labforge::ui {

class Features {
public:
  explicit Features(QwtPlot*plot, size_t steps_shown = DEFAULT_SAMPLES_SHOWN);
  virtual ~Features();

  void clear();
  void addSample(size_t n);
protected:
  void init();
private:
  std::deque<double> m_samples;
  size_t m_num_sample;
  const size_t m_steps_shown;
  std::unique_ptr<QPolygonF> m_points;
  QwtPlotCurve m_curve;
  QwtPlot *m_plot;
};

}

#endif // _features_hpp_