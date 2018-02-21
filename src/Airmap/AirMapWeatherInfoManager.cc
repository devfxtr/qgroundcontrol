/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "AirMapWeatherInfoManager.h"
#include "AirMapManager.h"

#define WEATHER_UPDATE_DISTANCE 50000                   //-- 50km threshold for weather updates
#define WEATHER_UPDATE_TIME     30 * 60 * 60 * 1000     //-- 30 minutes threshold for weather updates

using namespace airmap;

AirMapWeatherInfoManager::AirMapWeatherInfoManager(AirMapSharedState& shared, QObject *parent)
    : AirspaceWeatherInfoProvider(parent)
    , _valid(false)
    , _shared(shared)
{
}

void
AirMapWeatherInfoManager::setROI(const QGCGeoBoundingCube& roi)
{
    //-- If first time or we've moved more than WEATHER_UPDATE_DISTANCE, ask for weather updates.
    if(!_lastRoiCenter.isValid() || _lastRoiCenter.distanceTo(roi.center()) > WEATHER_UPDATE_DISTANCE) {
        _lastRoiCenter = roi.center();
        _requestWeatherUpdate(_lastRoiCenter);
        _weatherTime.start();
    } else {
        //-- Check weather once every WEATHER_UPDATE_TIME
        if(_weatherTime.elapsed() > WEATHER_UPDATE_TIME) {
            _requestWeatherUpdate(roi.center());
            _weatherTime.start();
        }
    }
}

void
AirMapWeatherInfoManager::_requestWeatherUpdate(const QGeoCoordinate& coordinate)
{
    qCDebug(AirMapManagerLog) << "Request Weather";
    if (!_shared.client()) {
        qCDebug(AirMapManagerLog) << "No AirMap client instance. Not updating Weather information";
        _valid = false;
        emit weatherChanged();
        return;
    }
    Status::GetStatus::Parameters params;
    params.longitude= coordinate.longitude();
    params.latitude = coordinate.latitude();
    params.weather  = true;
    _shared.client()->status().get_status_by_point(params, [this, coordinate](const Status::GetStatus::Result& result) {
        if (result) {
            _weather = result.value().weather;
            _valid  = true;
            if(_weather.icon.empty()) {
                _icon = QStringLiteral("qrc:/airmapweather/unknown.svg");
            } else {
                _icon = QStringLiteral("qrc:/airmapweather/") + QString::fromStdString(_weather.icon) + QStringLiteral(".svg");
            }
            qCDebug(AirMapManagerLog) << "Weather Info: " << _valid << "Icon:" << QString::fromStdString(_weather.icon) << "Condition:" << QString::fromStdString(_weather.condition) << "Temp:" << _weather.temperature;
        } else {
            _valid  = false;
            qCDebug(AirMapManagerLog) << "Request Weather Failed";
        }
        emit weatherChanged();
    });
}