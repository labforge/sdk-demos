#include "io/calib.hpp"
#include <nlohmann/json.hpp>
#include <yaml-cpp/yaml.h>
#include <fstream>
#include <vector>

using json = nlohmann::json;
using namespace std;

bool readCalibDotIOParameters(const QString &filePath,
                              std::map<QString,double> &kp) {

  if(!filePath.endsWith(".json", Qt::CaseInsensitive)){
    return false;
  }

  std::ifstream ifs(filePath.toStdString());
  json jsonStruct;

  try {
    ifs >> jsonStruct;

    if(jsonStruct["Calibration"]["cameras"][0]["model"]["polymorphic_name"] != "libCalib::CameraModelOpenCV"){
      return false;
    }

    uint32_t nCameras = (uint32_t)jsonStruct["Calibration"]["cameras"].size();
    if (nCameras < 1) {
      return false;
    }

    auto camera = jsonStruct["Calibration"]["cameras"][0]["model"]["ptr_wrapper"]["data"]["CameraModelCRT"]["CameraModelBase"];
    kp["kWidth"] = camera["imageSize"]["width"];
    kp["kHeight"] = camera["imageSize"]["height"];

    for (uint32_t i = 0; i < nCameras; ++i) {
      QString id = QString::number(i);
      auto intrinsics = jsonStruct["Calibration"]["cameras"][i]["model"]["ptr_wrapper"]["data"]["parameters"];

      kp["fx" + id] = intrinsics["f"]["val"];
      kp["fy" + id] = intrinsics["f"]["val"];
      kp["cx" + id] = intrinsics["cx"]["val"];
      kp["cy" + id] = intrinsics["cy"]["val"];
      kp["k1" + id] = intrinsics["k1"]["val"];
      kp["k2" + id] = intrinsics["k2"]["val"];
      kp["k3" + id] = intrinsics["k3"]["val"];
      kp["p1" + id] = intrinsics["p1"]["val"];
      kp["p2" + id] = intrinsics["p2"]["val"];

      auto transform = jsonStruct["Calibration"]["cameras"][i]["transform"];
      auto r = transform["rotation"];
      auto t = transform["translation"];
      kp["rx" + id] = r["rx"];
      kp["ry" + id] = r["ry"];
      kp["rz" + id] = r["rz"];
      kp["tx" + id] = t["x"];
      kp["ty" + id] = t["y"];
      kp["tz" + id] = t["z"];
    }

  } catch (...) {
    return false;
  }

  return true;
}

bool readYAMLParameters(const QString &filePath,
                         std::map<QString,double> &kp) {

  if((!filePath.endsWith(".yaml", Qt::CaseInsensitive)) &&
    (!filePath.endsWith(".yml", Qt::CaseInsensitive))){
    return false;
  }

  try {
    YAML::Node calib = YAML::LoadFile(filePath.toStdString());
    uint32_t nCameras =  (uint32_t)calib.size();

    if (nCameras < 1) {
      return false;
    }

    uint32_t tvec_count = 0;
    uint32_t rvec_count = 0;
    for(uint32_t i = 0; i < nCameras; ++i){
      QString id = QString::number(i);
      auto cam = "cam" + id.toStdString();

      if(!calib[cam]) return false;

      kp["fx" + id] = calib[cam]["fx"].as<double>();
      kp["fy" + id] = calib[cam]["fy"].as<double>();
      kp["cx" + id] = calib[cam]["cx"].as<double>();
      kp["cy" + id] = calib[cam]["cy"].as<double>();

      kp["k1" + id] = calib[cam]["k1"].as<double>();
      kp["k2" + id] = calib[cam]["k2"]?calib[cam]["k2"].as<double>():0.0;
      kp["k3" + id] = calib[cam]["k3"]?calib[cam]["k2"].as<double>():0.0;
      kp["p1" + id] = calib[cam]["p1"]?calib[cam]["p1"].as<double>():0.0;
      kp["p2" + id] = calib[cam]["p2"]?calib[cam]["p2"].as<double>():0.0;

      std::vector<double> tvec = {0.0,0.0,0.0};
      std::vector<double> rvec = {0.0,0.0,0.0};
      if(calib[cam]["tvec"]){
        tvec = calib[cam]["tvec"].as<std::vector<double>>();
        tvec_count += 1;
      }
      if(calib[cam]["rvec"]){
        rvec = calib[cam]["rvec"].as<std::vector<double>>();
        rvec_count += 1;
      }

      kp["tx" + id] = tvec[0];
      kp["ty" + id] = tvec[1];
      kp["tz" + id] = tvec[2];
      kp["rx" + id] = rvec[0];
      kp["ry" + id] = rvec[1];
      kp["rz" + id] = rvec[2];

      kp["kWidth"] = calib[cam]["width"].as<double>();
      kp["kHeight"] = calib[cam]["height"].as<double>();
    }

    if(tvec_count < (nCameras - 1) || rvec_count < (nCameras - 1)){
      return false;
    }

  } catch (...) {
    return false;
  }

  return true;
}

bool loadCalibration(const QString &filePath,
                     std::map<QString,double> &kp) {
  if(readCalibDotIOParameters(filePath, kp)) return true;
  if(readYAMLParameters(filePath, kp)) return true;
  return false;
}
