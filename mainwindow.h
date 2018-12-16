#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QGeoCoordinate>

struct Position
{
    Position() : latitude(0.0), longitude(0.0) {}
    Position(double latitudeIn, double longitudeIn) : latitude(latitudeIn), longitude(longitudeIn) {}

    Position operator-(const Position& other) { return Position(std::abs(longitude - other.longitude), std::abs(latitude - other.latitude)); }

    double latitude, longitude;
};

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void onResult(class QNetworkReply* reply);

    void on_updateButton_clicked();

    void on_goToFlightButton_clicked();

private:
    Ui::MainWindow *ui;

    void updateData();

    class QNetworkReply* reply_;

    const QJsonArray getClosestFlight(const QJsonArray& flights);

    void addClosestFlightDetailsToListWidget(const QJsonArray& flight);

    QString callsign_;

    const QGeoCoordinate myPosition_{59.744494, 10.214935};
};

#endif // MAINWINDOW_H
