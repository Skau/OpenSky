#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

#include <QGeoCoordinate>

typedef std::pair<int, double> IndexDistancePair;

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

    const QJsonArray getClosestFlight(const QJsonArray& flights) const;

    void addClosestFlightDetailsToListWidget(const QJsonArray& flight) const;

    double clamp(double value, double low, double high);

    static bool compare(const IndexDistancePair& pair1, const IndexDistancePair& pair2) { return pair1.second < pair2.second; }

    QString callsign_;

    QGeoCoordinate myPosition_;
};

#endif // MAINWINDOW_H
