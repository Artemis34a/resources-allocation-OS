//
// Created by artemis on 13/11/2025.
//
#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <iomanip>
#include <thread>
#include <chrono>
#include <algorithm>

using namespace std;

struct Process {
    string name;
    int demand;
    int remaining;
    int priority;
    bool finished = false;
};

struct Queue {
    string name;
    vector<Process> processes;
    double weight;
    string policy; // "RR" or "FIFO"
    int rrIndex = 0;
};

class ResourceAllocator {
    double totalResource;
    vector<Queue> queues;
    ofstream logFile;

public:
    ResourceAllocator(double totalRes) : totalResource(totalRes) {
        logFile.open("allocation_log.txt");
        logFile << "=== Simulation Log Start ===\n";
    }

    ~ResourceAllocator() {
        logFile << "=== Simulation End ===\n";
        logFile.close();
    }

    void addQueue(const Queue &q) { queues.push_back(q); }

    void simulate(int cycles, double unit) {
        for (int cycle = 1; cycle <= cycles; ++cycle) {
            clearScreen();
            cout << "\n=== Cycle " << cycle << " ===\n";
            logFile << "\n=== Cycle " << cycle << " ===\n";

            // 1️. Allocation inter-files
            double totalWeight = 0;
            for (auto &q : queues) totalWeight += q.weight;
            // Il s'agit de la formule génerale d'allocation des ressouces
            for (auto &q : queues) {
                double quota = (q.weight / totalWeight) * totalResource;
                allocateInQueue(q, quota, unit);
            }

            displayState();
            this_thread::sleep_for(chrono::milliseconds(1200));
        }
    }

private:
    void allocateInQueue(Queue &q, double quota, double unit) {
        cout << "\n[" << q.name << "]  (" << q.policy << ") : Quota " << quota << "\n";
        logFile << "\n[" << q.name << "]  (" << q.policy << ") : Quota " << quota << "\n";

        if (q.policy == "RR") roundRobin(q, quota, unit);
        else fifo(q, quota, unit);
    }

    void roundRobin(Queue &q, double quota, double unit) {
        int n = q.processes.size();
        int served = 0;

        while (quota > 0 && served < n) {
            Process &p = q.processes[q.rrIndex];

            if (!p.finished) {
                double alloc = min((double)p.remaining, unit);
                alloc = min(alloc, quota);
                p.remaining -= alloc;
                quota -= alloc;
                logAllocation(q.name, p.name, alloc);
                if (p.remaining <= 0) {
                    p.finished = true;
                    cout << " ⚙️ ﮩ٨ـﮩﮩ٨ـﮩ٨ـﮩﮩ٨ـ   " << p.name << " terminé.\n";
                    logFile << " ⚙️ ﮩ٨ـﮩﮩ٨ـﮩ٨ـﮩﮩ٨ـ   " << p.name << " terminé.\n";
                }
            }

            q.rrIndex = (q.rrIndex + 1) % n;
            served++;
        }
    }

    void fifo(Queue &q, double quota, double unit) {
        for (auto &p : q.processes) {
            if (quota <= 0) break;
            if (!p.finished) {
                double alloc = min((double)p.remaining, unit);
                alloc = min(alloc, quota);
                p.remaining -= alloc;
                quota -= alloc;
                logAllocation(q.name, p.name, alloc);
                if (p.remaining <= 0) {
                    p.finished = true;
                    cout << " ⚙️ ﮩ٨ـﮩﮩ٨ـﮩ٨ـﮩﮩ٨ـ   " << p.name << " terminé.\n";
                    logFile << " ⚙️ ﮩ٨ـﮩﮩ٨ـﮩ٨ـﮩﮩ٨ـ   " << p.name << " terminé.\n";
                }
            }
        }
    }

    void displayState() {
        cout << "\n--- État actuel des files ---\n";
        for (auto &q : queues) {
            cout << q.name << " :\n";
            for (auto &p : q.processes) {
                cout << "   • " << setw(8) << p.name
                     << " | restant: " << setw(4) << p.remaining
                     << " | " << (p.finished ? "✅" : "⏳") << "\n";
            }
        }
    }

    void logAllocation(const string &queue, const string &process, double alloc) {
        cout << " ➜ " << process << " reçoit " << alloc << " unités\n";
        logFile << " ➜ " << process << " reçoit " << alloc << " unités\n";
    }

    void clearScreen() {
#ifdef _WIN32
        system("cls");
#else
        system("clear");
#endif
    }
};

// === MAIN ===
int main() {
    ResourceAllocator allocator(100.0);

    Queue q1 = {"File 1 (VVIP)", {
        {"P1", 50, 50, 1}, {"P2", 30, 30, 1}
    }, 0.5, "RR"};

    Queue q2 = {"File 2 (VIP)", {
        {"P3", 60, 60, 2}, {"P4", 40, 40, 2}
    }, 0.3, "RR"};

    Queue q3 = {"File 3 (CLASSIC)", {
        {"P5", 80, 80, 3}, {"P6", 20, 20, 3}
    }, 0.2, "FIFO"};

    allocator.addQueue(q1);
    allocator.addQueue(q2);
    allocator.addQueue(q3);

    allocator.simulate(15, 10.0);

    cout << "\nSimulation terminée. Voir 'allocation_log.txt' pour le journal complet.\n";
    return 0;
}
