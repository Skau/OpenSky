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
#include <vector>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow), callsign_{""}, myPosition_{QGeoCoordinate(0,0)}
{
    ui->setupUi(this);

    ui->latitudeLineEdit->setValidator(new QDoubleValidator(-90, 90, 6, this));
    ui->longitudeLineEdit->setValidator(new QDoubleValidator(-180, 180, 6, this));

    ui->latitudeLineEdit->setText("59.744473");
    ui->longitudeLineEdit->setText("10.214885");

    setWindowTitle("Get nearest flight");
    ui->goToFlightButton->setEnabled(false);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::updateData()
{
    ui->listWidget->clear();
    if(ui->goToFlightButton->isEnabled())
    {
        ui->goToFlightButton->setEnabled(false);
    }

    double latitude = ui->latitudeLineEdit->text().toDouble();
    double longitude = ui->longitudeLineEdit->text().toDouble();

    if(!ui->latitudeLineEdit->text().size() || latitude <= -90 || latitude >= 90)
    {
        ui->statusBar->showMessage("Latitude invalid! (Min -90° and max 90°", 2000);
        return;
    }
    else if(!ui->longitudeLineEdit->text().size() || longitude <= -180 || longitude >= 180)
    {
        ui->statusBar->showMessage("Longitude invalid! (Min -180° and max 180°", 2000);
        return;
    }

    QString url = "https://opensky-network.org/api/states/all?";
    url +=
            "lamin=" + QString::number(clamp(latitude-2, -90, 90)) +
            "&lomin=" + QString::number(clamp(longitude-4, -180, 180)) +
            "&lamax=" + QString::number(clamp(latitude+2, -90, 90)) +
            "&lomax=" + QString::number(clamp(longitude+4, -180, 180));

    myPosition_ = QGeoCoordinate(latitude, longitude);

    QNetworkAccessManager* manager = new QNetworkAccessManager(this);

    reply_ = manager->get(QNetworkRequest(QUrl(url)));

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

    if(callsign_ != "")
    {
        ui->goToFlightButton->setEnabled(true);
    }
}

const QJsonArray MainWindow::getClosestFlight(const QJsonArray& flights)
{
    std::vector<IndexDistancePair> foundFlights;

    for(int i = 0; i < flights.size(); ++i)
    {
        QJsonArray flight = flights[i].toArray();

        if(flight[1].toString().size())
        {
            if(flight[5].toDouble() > 0 && flight[6].toDouble() > 0)
            {
                QGeoCoordinate flightPosition(flight[6].toDouble(), flight[5].toDouble());

                foundFlights.push_back(std::make_pair(i, flightPosition.distanceTo(myPosition_)));
            }
        }
    }

    auto it = std::min_element(foundFlights.begin(), foundFlights.end(), compare);

    return flights[it->first].toArray();
}

void MainWindow::addClosestFlightDetailsToListWidget(const QJsonArray& flight)
{
    QGeoCoordinate position(flight[6].toDouble(), flight[5].toDouble(), flight[7].toDouble());

    QString string;
    string = "Closest flight is " + flight[1].toString();
    ui->listWidget->addItem(string);

    string = "Latitude: " + QString::number(position.latitude()) + "°, longitude: " + QString::number(position.longitude()) + "°";
    ui->listWidget->addItem(string);

    string = "Altidude: " + QString::number(position.altitude()) + "m";
    ui->listWidget->addItem(string);

    string = "Current speed: " + QString::number((flight[9].toDouble() * 18) / 5) + " km/h";
    ui->listWidget->addItem(string);

    string = "Origin Country: " + flight[2].toString();
    ui->listWidget->addItem(string);

    std::time_t time = flight[3].toInt();
    string = "Position last updated: " + QString(std::asctime(std::localtime(&time)));
    ui->listWidget->addItem(string);
}

double MainWindow::clamp(double value, double low, double high)
{
    if(value < low)
    {
        value = low;
    }
    else if(value > high)
    {
        value = high;
    }
    return value;
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

