#include <QString>
#include <map>

bool loadCalibration(const QString &filePath,
                     std::map<QString, double> &kp);