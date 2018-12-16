#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QDebug>
#include <QUrl>
#include <QtNetwork/qnetworkaccessmanager.h>
#include <QtNetwork/qnetworkrequest.h>
#include <QtNetwork/QNetworkReply>
#include <qjsondocument.h>
#include <qjsonobject.h>
#include <qjsonarray.h>
#include <math.h>
#include <QDesktopServices>
#include <ctime>

#define pi 3.14159265358979323846

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow), callsign_{""}
{
    ui->setupUi(this);

    setWindowTitle("Get nearest flight");
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::updateData()
{
    QNetworkAccessManager* manager = new QNetworkAccessManager(this);

    reply_ = manager->get(QNetworkRequest(QUrl("https://opensky-network.org/api/states/all?lamin=57&lomin=2&lamax=72&lomax=32")));

    connect(manager, SIGNAL(finished(QNetworkReply*)), this, SLOT(onResult(QNetworkReply*)));

    manager = nullptr;
}

void MainWindow::onResult(QNetworkReply* reply)
{
    if(reply_->error() != QNetworkReply::NoError)
    {
        qDebug() << reply_->errorString();
        return;
    }

    QString data = static_cast<QString>(reply->readAll());
    QJsonDocument doc = QJsonDocument::fromJson(data.toUtf8());
    QJsonObject jsonObject = doc.object();

    auto it = jsonObject.find("states");
    if(it->isArray())
    {
        QJsonArray flights = it->toArray();

        QJsonArray closestFlight = getClosestFlight(flights);

        if(callsign_ != closestFlight[1].toString())
        {
            callsign_ = closestFlight[1].toString();
            addClosestFlightDetailsToListWidget(closestFlight);
        }
    }
}

const QJsonArray MainWindow::getClosestFlight(const QJsonArray& flights)
{
    QJsonArray closestFlight = flights[0].toArray();
    QGeoCoordinate closestPosition(closestFlight[6].toDouble(), closestFlight[5].toDouble());
    double shortestDistanceFromLocation = closestPosition.distanceTo(myPosition_), currentFlightDistanceFromLocation;

    for(int i = 0; i < flights.size(); ++i)
    {
        QJsonArray flight = flights[i].toArray();

        if(flight[1].toString().size())
        {
            if(flight[5].toDouble() > 0 && flight[6].toDouble() > 0)
            {
                QGeoCoordinate flightPosition(flight[6].toDouble(), flight[5].toDouble());

                currentFlightDistanceFromLocation = flightPosition.distanceTo(myPosition_);

                if(currentFlightDistanceFromLocation < shortestDistanceFromLocation)
                {
                    closestFlight = flight;
                    closestPosition = flightPosition;
                    shortestDistanceFromLocation = currentFlightDistanceFromLocation;
                }
            }
        }
    }
    return closestFlight;
}

void MainWindow::addClosestFlightDetailsToListWidget(const QJsonArray& flight)
{
    ui->listWidget->clear();

    QGeoCoordinate position(flight[5].toDouble(), flight[6].toDouble(), flight[7].toDouble());

    QString string;
    string = "Closest flight is " + flight[1].toString();
    ui->listWidget->addItem(string);

    string = "Latitude: " + QString::number(position.latitude()) + "°, longitude: " + QString::number(position.longitude()) + "°";
    ui->listWidget->addItem(string);

    string = "Altidude: " + QString::number(position.altitude() / 1000) + "km";
    ui->listWidget->addItem(string);

    string = "Current speed: " + QString::number((flight[9].toDouble() * 18) / 5) + " km/h";
    ui->listWidget->addItem(string);

    string = "Origin Country: " + flight[2].toString();
    ui->listWidget->addItem(string);

    std::time_t time = flight[3].toInt();
    string = "Position last updated: " + QString(std::asctime(std::localtime(&time)));
    ui->listWidget->addItem(string);
}

void MainWindow::on_updateButton_clicked()
{
    updateData();
}

void MainWindow::on_goToFlightButton_clicked()
{
    if(callsign_.size())
    {
        callsign_ = callsign_.simplified();
        callsign_.replace(" ", "");

        QString link = "https://www.flightradar24.com/" + callsign_;
        QDesktopServices::openUrl(QUrl(link));
    }
}

