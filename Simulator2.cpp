//
// Created by artemis on 13/11/2025.
//
#include <iostream>
#include <vector>
#include <string>
#include <iomanip>
#include <algorithm>
using namespace std;

struct Process {
    string name;
    double demand;
    double remaining;
    double weight;
    int waitTime = 0;

    Process(string n, double d, double w)
        : name(n), demand(d), remaining(d), weight(w) {}
};

struct Queue {
    string name;
    double baseWeight;
    double cap;
    double aging = 0.0;  // facteur d'augmentation de priorité
    vector<Process> processes;

    Queue(string n, double w, double c)
        : name(n), baseWeight(w), cap(c) {}

    bool allFinished() const {
        for (auto &p : processes)
            if (p.remaining > 0) return false;
        return true;
    }
};

class DynamicScheduler {
private:
    vector<Queue> queues;
    double totalResource;
    double agingRate;
    double redistributionFactor;

public:
    DynamicScheduler(double totalRes, double ageRate = 0.1, double redist = 0.2)
        : totalResource(totalRes), agingRate(ageRate), redistributionFactor(redist) {}

    void addQueue(const Queue &q) {
        queues.push_back(q);
    }

    void run(int cycles) {
        cout << fixed << setprecision(2);

        for (int t = 1; t <= cycles; ++t) {
            cout << "\n=== Cycle " << t << " ===\n";

            // Étape 1 : Calcul du poids effectif (avec aging)
            double totalWeight = 0.0;
            for (auto &q : queues) {
                if (!q.allFinished()) {
                    totalWeight += (q.baseWeight + q.aging);
                }
            }

            // Étape 2 : Allocation initiale
            for (auto &q : queues) {
                if (q.allFinished()) continue;

                double effectiveWeight = q.baseWeight + q.aging;
                double alloc = totalResource * (effectiveWeight / totalWeight);
                alloc = min(q.cap, alloc);

                cout << "\n[Queue " << q.name << "] reçoit " << alloc << " unités.\n";

                // Étape 3 : Distribution interne (Round Robin simplifié)
                int activeCount = 0;
                for (auto &p : q.processes)
                    if (p.remaining > 0) activeCount++;

                if (activeCount == 0) continue;

                double perProcess = alloc / activeCount;

                for (auto &p : q.processes) {
                    if (p.remaining > 0) {
                        double used = min(p.remaining, perProcess);
                        p.remaining -= used;
                        p.waitTime = 0;
                        cout << "  " << p.name << " utilise " << used
                             << " (reste: " << p.remaining << ")\n";
                    } else {
                        p.waitTime++;
                    }
                }

                // Étape 4 : Famine (si la file n’a pas eu assez de ressources)
                if (alloc < 0.1 * q.cap) {
                    q.aging += agingRate;
                    cout << "  ⚠️  Famine détectée → aging augmenté à " << q.aging << "\n";
                } else {
                    // Réinitialisation progressive de l'aging
                    q.aging = max(0.0, q.aging - 0.05);
                }
            }

            // Étape 5 : Redistribution (files terminées)
            double unused = 0.0;
            for (auto &q : queues)
                if (q.allFinished()) unused += q.cap * redistributionFactor;

            if (unused > 0) {
                cout << "\nRedistribution de " << unused << " unités inutilisées.\n";
                for (auto &q : queues)
                    if (!q.allFinished())
                        q.cap += unused / queues.size();
            }

            // Étape 6 : Affichage de l’état global
            cout << "\nÉtat global des files :\n";
            for (auto &q : queues) {
                cout << "  " << q.name << " → aging=" << q.aging
                     << ", cap=" << q.cap << "\n";
            }
        }
    }
};

int main() {
    DynamicScheduler scheduler(100.0);

    Queue high("HIGH", 3.0, 50.0);
    high.processes = { Process("P1", 40, 1.0), Process("P2", 25, 1.0) };

    Queue medium("MEDIUM", 2.0, 35.0);
    medium.processes = { Process("P3", 30, 1.0), Process("P4", 25, 1.0) };

    Queue low("LOW", 1.0, 25.0);
    low.processes = { Process("P5", 20, 1.0), Process("P6", 15, 1.0) };

    scheduler.addQueue(high);
    scheduler.addQueue(medium);
    scheduler.addQueue(low);

    scheduler.run(15);
}
