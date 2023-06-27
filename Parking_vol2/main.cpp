#include <QApplication>
#include <QWidget>
#include <QVBoxLayout>
#include <QPushButton>
#include <QMessageBox>
#include <QInputDialog>
#include <QTimer>
#include <QFile>
#include <QTextStream>
#include <cstdlib>
#include <vector>
#include <chrono>
#include <cmath>

struct Car {
    std::string registration;
    std::chrono::time_point<std::chrono::system_clock> entryTime;
    std::chrono::time_point<std::chrono::system_clock> lastFeeCalculationTime;
    double totalFee;
};

struct Price {
    std::string service;
    double cost;
};

std::vector<Car*> parkedCars;
std::vector<Price> prices;

class ParkingWidget : public QWidget {
public:
    ParkingWidget(QWidget *parent = nullptr) : QWidget(parent) {
        QVBoxLayout *layout = new QVBoxLayout(this);

        QPushButton *parkButton = new QPushButton("Zaparkuj Samochód", this);
        layout->addWidget(parkButton);
        connect(parkButton, &QPushButton::clicked, this, &ParkingWidget::parkCar);

        QPushButton *availableSpacesButton = new QPushButton("Dostępne Miejsca", this);
        layout->addWidget(availableSpacesButton);
        connect(availableSpacesButton, &QPushButton::clicked, this, &ParkingWidget::showAvailableSpaces);

        QPushButton *priceListButton = new QPushButton("Cennik", this);
        layout->addWidget(priceListButton);
        connect(priceListButton, &QPushButton::clicked, this, &ParkingWidget::showPriceList);

        payAndExitButton = new QPushButton("Zapłać i Wyjedź", this);
        layout->addWidget(payAndExitButton);
        connect(payAndExitButton, &QPushButton::clicked, this, &ParkingWidget::payAndExitCar);
        payAndExitButton->setEnabled(false);

        QPushButton *exitButton = new QPushButton("Wyjście", this);
        layout->addWidget(exitButton);
        connect(exitButton, &QPushButton::clicked, qApp, QApplication::quit);

        setLayout(layout);

        QTimer *timer = new QTimer(this);
        connect(timer, &QTimer::timeout, this, &ParkingWidget::calculateFees);
        timer->start(1000);
    }

private slots:
    void parkCar() {
        bool ok;
        QString registration = QInputDialog::getText(this, "Zaparkuj Samochód", "Podaj numer rejestracyjny:", QLineEdit::Normal, "", &ok);
        if (ok && !registration.isEmpty()) {
            std::string registrationStr = registration.toStdString();
            if (findCar(registrationStr) != nullptr) {
                QMessageBox::warning(this, "Błąd", "Samochód o podanym numerze rejestracyjnym już jest zaparkowany.");
            } else {
                Car* car = new Car;
                car->registration = registrationStr;
                car->entryTime = std::chrono::system_clock::now();
                car->lastFeeCalculationTime = car->entryTime;
                car->totalFee = 0.0;
                parkedCars.push_back(car);
                QMessageBox::information(this, "Sukces", "Samochód został zaparkowany.");
                updatePayAndExitButtonState();
                saveCarsToFile();
            }
        }
    }

    void showAvailableSpaces() {
        int availableSpaces = getAvailableSpaces();
        QString message = "Dostępne Miejsca Parkingowe: " + QString::number(availableSpaces);
        QMessageBox::information(this, "Dostępne Miejsca", message);
    }

    void showPriceList() {
        QString message = "Cennik:\n";
        for (const auto& price : prices) {
            message += "Usługa: " + QString::fromStdString(price.service) + ", Koszt: " + QString::number(price.cost*2) + " zł/s\n";
        }
        QMessageBox::information(this, "Cennik", message);
    }

    void payAndExitCar() {
        QStringList carList;
        for (const auto& car : parkedCars) {
            carList.append(QString::fromStdString(car->registration));
        }

        bool ok;
        QString registration = QInputDialog::getItem(this, "Zapłać i Wyjedź", "Wybierz samochód:", carList, 0, false, &ok);

        if (ok && !registration.isEmpty()) {
            std::string registrationStr = registration.toStdString();
            Car* car = findCar(registrationStr);
            if (car != nullptr) {
                double totalFee = calculateTotalFee(car->registration);
                QString message = "Opłata do zapłacenia: " + QString::number(totalFee) + " PLN";
                QMessageBox::information(this, "Zapłać i Wyjedź", message);
                updateTotalFee(car, totalFee);
                removeCar(car);
                updatePayAndExitButtonState();
                saveCarsToFile();
            } else {
                QMessageBox::warning(this, "Błąd", "Nie znaleziono samochodu o podanym numerze rejestracyjnym.");
            }
        }
    }

    void calculateFees() {
        std::chrono::time_point<std::chrono::system_clock> currentTime = std::chrono::system_clock::now();
        for (auto& car : parkedCars) {
            std::chrono::duration<double> duration = currentTime - car->lastFeeCalculationTime;
            double elapsedSeconds = duration.count();
            double rate = prices[0].cost;
            double totalFee = car->totalFee + (elapsedSeconds * rate);
            car->totalFee = totalFee;
            car->lastFeeCalculationTime = currentTime;
        }
        saveCarsToFile();
    }

private:
    Car* findCar(const std::string& registration) {
        for (const auto& car : parkedCars) {
            if (car->registration == registration) {
                return car;
            }
        }
        return nullptr;
    }

    int getAvailableSpaces() {
        int totalSpaces = 100;
        int occupiedSpaces = parkedCars.size();
        return totalSpaces - occupiedSpaces;
    }

    double calculateTotalFee(const std::string& registration) {
        Car* car = findCar(registration);
        if (car != nullptr) {
            std::chrono::time_point<std::chrono::system_clock> currentTime = std::chrono::system_clock::now();
            std::chrono::duration<double> duration = currentTime - car->entryTime;
            double elapsedSeconds = duration.count();
            double rate = prices[0].cost;
            double totalFee = car->totalFee + (elapsedSeconds * rate);
            totalFee = std::round(totalFee);
            return totalFee;
        }
        return 0.0;
    }


    void updateTotalFee(Car* car, double totalFee) {
        car->totalFee = totalFee;
    }

    void removeCar(Car* car) {
        parkedCars.erase(std::remove(parkedCars.begin(), parkedCars.end(), car), parkedCars.end());
        delete car;
    }

    void saveCarsToFile() {
        QFile file("parked_cars.txt");
        if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QTextStream out(&file);
            for (const auto& car : parkedCars) {
                out << "Numer rejestracyjny: " << QString::fromStdString(car->registration) << "\n";
                out << "Czas wjazdu: " << car->entryTime.time_since_epoch().count() << "\n";
                out << "Całkowita opłata: " << car->totalFee << "\n";
                out << "====================\n";
            }
            file.close();
        }
    }

    void updatePayAndExitButtonState() {
        payAndExitButton->setEnabled(!parkedCars.empty());
    }

    QPushButton *payAndExitButton;
};

int main(int argc, char *argv[]) {
    srand(time(nullptr));

    QApplication app(argc, argv);

    prices.push_back({ "Postój ", 1});

    ParkingWidget parkingWidget;
    parkingWidget.show();

    return app.exec();
}
