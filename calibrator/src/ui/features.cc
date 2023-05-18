#include "ui/features.hpp"
#include "qwt_symbol.h"
#include <stdexcept>
#include <iostream>

using namespace std;
using namespace labforge::ui;

Features::Features(QwtPlot*plot, size_t steps_shown) : m_plot(plot), m_steps_shown(steps_shown), m_num_sample(0) {
  if(plot == nullptr) {
    throw runtime_error("Plot cannot be NULL");
  }
  m_curve.setPen(Qt::blue, 4);
  QwtSymbol *symbol = new QwtSymbol(
          QwtSymbol::Ellipse,
          QBrush( Qt::yellow ),
          QPen( Qt::red, 2 ),
          QSize( 8, 8 ) );

  m_curve.setSymbol(symbol);
  m_curve.attach(m_plot);
  init();
}

Features::~Features() noexcept {
  m_samples.clear();
}

void Features::init() {
  m_plot->setAxisTitle(QwtPlot::xBottom, "Frame");
  m_plot->setAxisTitle(QwtPlot::yLeft, "# Features");
  m_plot->setAxisScale(QwtPlot::xBottom, 0, m_steps_shown);
  m_plot->setCanvasBackground(Qt::white);
}

void Features::addSample(size_t n) {
  size_t i = 0;
  while(m_samples.size() >= m_steps_shown) {
    m_samples.pop_front();
    m_plot->setAxisScale(QwtPlot::xBottom, m_num_sample-m_steps_shown+1, m_num_sample+1);
    i = m_num_sample-m_steps_shown+1;
  }
  m_samples.push_back(n);
  m_points = make_unique<QPolygonF>();
  for(auto &d : m_samples) {
    *m_points << QPointF(i++, d);
  }
  m_num_sample++;
  m_curve.setSamples(*m_points);
  m_plot->setEnabled(true);
  m_plot->replot();
}

void Features::clear() {
  m_samples.clear();
  m_points = make_unique<QPolygonF>();
  m_curve.setSamples(*m_points);
  m_plot->replot();
  m_plot->setEnabled(false);
  m_plot->setAxisScale(QwtPlot::xBottom, 0, m_steps_shown);
  m_num_sample = 0;
}