#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <iomanip>
#include <thread>
#include <chrono>
#include <algorithm>
#include <map>
#include <sstream>
#include <ctime>

using namespace std;

// ==================== STRUCTURES ====================
struct Process {
    string name;
    double demand;
    double remaining;
    int priority;
    bool finished = false;
    double allocated = 0.0;
    int startCycle = -1;
    int endCycle = -1;
};

struct Queue {
    string name;
    vector<Process> processes;
    double weight;
    string policy;
    int rrIndex = 0;
    double totalAllocated = 0.0;
};

struct CycleStats {
    int cycleNumber;
    map<string, double> queueAllocations;
    map<string, vector<pair<string, double>>> processAllocations;
    int activeProcesses;
    double totalAllocated;
};

// ==================== CLASSE UTILITAIRE ====================
class Display {
public:
    static void clearScreen() {
        #ifdef _WIN32
            system("cls");
        #else
            system("clear");
        #endif
    }

    static void printHeader(const string& title) {
        cout << "\n";
        cout << "╔════════════════════════════════════════════════════════════════════╗\n";
        cout << "║" << setw(36 + title.length()/2) << title 
             << setw(36 - title.length()/2) << "║\n";
        cout << "╚════════════════════════════════════════════════════════════════════╝\n";
    }

    static void printSeparator() {
        cout << "────────────────────────────────────────────────────────────────────\n";
    }

    static void printSubHeader(const string& text) {
        cout << "\n┌─ " << text << " ─┐\n";
    }

    static string getStatusEmoji(bool finished) {
        return finished ? "✅" : "⏳";
    }

    static string getPriorityEmoji(int priority) {
        switch(priority) {
            case 1: return "🔴";
            case 2: return "🟠";
            case 3: return "🟢";
            default: return "⚪";
        }
    }

    static void printResourceGrid(double totalResource, const vector<Queue>& queues, int gridWidth = 50) {
        cout << "\n📊 GRILLE DE RESSOURCES (Total: " << totalResource << " unités)\n";
        cout << "┌";
        for(int i = 0; i < gridWidth; i++) cout << "─";
        cout << "┐\n│";

        int totalCells = gridWidth;
        map<string, int> queueCells;
        
        // Calculer le nombre de cellules par file
        double totalWeight = 0;
        for(const auto& q : queues) totalWeight += q.weight;
        
        for(const auto& q : queues) {
            queueCells[q.name] = (int)((q.weight / totalWeight) * totalCells);
        }

        // Afficher la grille
        for(const auto& q : queues) {
            int cells = queueCells[q.name];
            string emoji = q.name.find("VVIP") != string::npos ? "🔴" : 
                          q.name.find("VIP") != string::npos ? "🟠" : "🟢";
            
            for(int i = 0; i < cells; i++) {
                if(i == cells/2) cout << emoji;
                else cout << "█";
            }
        }
        cout << "│\n└";
        for(int i = 0; i < gridWidth; i++) cout << "─";
        cout << "┘\n";

        // Légende
        cout << "\n  ";
        for(const auto& q : queues) {
            string emoji = q.name.find("VVIP") != string::npos ? "🔴" : 
                          q.name.find("VIP") != string::npos ? "🟠" : "🟢";
            double quota = (q.weight / totalWeight) * totalResource;
            cout << emoji << " " << q.name << " (" << quota << " unités)  ";
        }
        cout << "\n";
    }

    static void printAllocationTable(const vector<Queue>& queues) {
        cout << "\n📋 TABLEAU D'ALLOCATION PAR FILE\n";
        cout << "╔════════════════════╦═══════════╦═══════════╦════════════╦════════╗\n";
        cout << "║ Processus          ║ Demande   ║ Restant   ║ Alloué     ║ État   ║\n";
        cout << "╠════════════════════╬═══════════╬═══════════╬════════════╬════════╣\n";

        for(const auto& q : queues) {
            cout << "║ " << setw(18) << left << q.name << " ║           ║           ║            ║        ║\n";
            for(const auto& p : q.processes) {
                cout << "║  ├─ " << setw(13) << left << p.name 
                     << " ║ " << setw(9) << right << fixed << setprecision(1) << p.demand
                     << " ║ " << setw(9) << p.remaining
                     << " ║ " << setw(10) << p.allocated
                     << " ║ " << setw(6) << getStatusEmoji(p.finished) << " ║\n";
            }
            if(&q != &queues.back()) {
                cout << "╟────────────────────╫───────────╫───────────╫────────────╫────────╢\n";
            }
        }
        cout << "╚════════════════════╩═══════════╩═══════════╩════════════╩════════╝\n";
    }

    static void printDetailedGrid(const vector<Queue>& queues, double totalResource) {
        cout << "\n🎯 OCCUPATION DÉTAILLÉE DES RESSOURCES\n\n";
        
        double totalWeight = 0;
        for(const auto& q : queues) totalWeight += q.weight;
        
        for(const auto& q : queues) {
            double quota = (q.weight / totalWeight) * totalResource;
            string emoji = q.name.find("VVIP") != string::npos ? "🔴" : 
                          q.name.find("VIP") != string::npos ? "🟠" : "🟢";
            
            cout << emoji << " " << q.name << " [" << q.policy << "] - Quota: " 
                 << fixed << setprecision(1) << quota << " unités\n";
            
            int barWidth = 40;
            int activeCount = 0;
            for(const auto& p : q.processes) {
                if(!p.finished) activeCount++;
            }
            
            for(const auto& p : q.processes) {
                cout << "  │ " << setw(10) << left << p.name << " ";
                
                int filled = 0;
                if(p.demand > 0) {
                    filled = (int)((p.allocated / p.demand) * barWidth);
                }
                
                cout << "[";
                for(int i = 0; i < barWidth; i++) {
                    if(i < filled) cout << "█";
                    else cout << "░";
                }
                cout << "] ";
                
                cout << fixed << setprecision(1) << p.allocated << "/" << p.demand;
                if(p.finished) cout << " ✅";
                cout << "\n";
            }
            cout << "\n";
        }
    }
};

// ==================== CLASSE PRINCIPALE ====================
class ResourceAllocator {
private:
    double totalResource;
    vector<Queue> queues;
    vector<CycleStats> history;
    ofstream logFile;
    ofstream jsonFile;
    int currentCycle = 0;

public:
    ResourceAllocator(double totalRes) : totalResource(totalRes) {
        time_t now = time(0);
        tm *ltm = localtime(&now);
        
        logFile.open("allocation_log.txt");
        jsonFile.open("allocation_data.json");
        
        logFile << "=== SIMULATION D'ALLOCATION DE RESSOURCES ===\n";
        logFile << "Date: " << asctime(ltm);
        logFile << "Ressource totale: " << totalResource << " unités\n\n";
        
        jsonFile << "{\n";
        jsonFile << "  \"simulation\": {\n";
        jsonFile << "    \"totalResource\": " << totalResource << ",\n";
        jsonFile << "    \"startTime\": \"" << asctime(ltm) << "\",\n";
        jsonFile << "    \"cycles\": [\n";
    }

    ~ResourceAllocator() {
        jsonFile << "    ]\n";
        jsonFile << "  }\n";
        jsonFile << "}\n";
        
        logFile.close();
        jsonFile.close();
    }

    void addQueue(const Queue &q) { 
        queues.push_back(q); 
    }

    void showInitialState() {
        Display::clearScreen();
        Display::printHeader("🚀 ÉTAT INITIAL DU SYSTÈME");
        
        cout << "\n📦 Configuration des files d'attente:\n\n";
        
        double totalWeight = 0;
        for(const auto& q : queues) totalWeight += q.weight;
        
        for(const auto& q : queues) {
            double quota = (q.weight / totalWeight) * totalResource;
            cout << "  • " << q.name << "\n";
            cout << "    ├─ Politique: " << q.policy << "\n";
            cout << "    ├─ Poids: " << fixed << setprecision(2) << q.weight 
                 << " (" << (q.weight/totalWeight*100) << "%)\n";
            cout << "    ├─ Quota: " << fixed << setprecision(1) << quota << " unités\n";
            cout << "    └─ Processus: " << q.processes.size() << "\n\n";
        }

        Display::printResourceGrid(totalResource, queues);
        Display::printAllocationTable(queues);

        logFile << "=== ÉTAT INITIAL ===\n";
        for(const auto& q : queues) {
            logFile << q.name << " (" << q.policy << "): " << q.processes.size() << " processus\n";
        }
        logFile << "\n";
    }

    bool allProcessesFinished() {
        for(const auto& q : queues) {
            for(const auto& p : q.processes) {
                if(!p.finished) return false;
            }
        }
        return true;
    }

    void simulate(double unit, int cycleDelay = 3000) {
        showInitialState();
        
        cout << "\n\n";
        Display::printSeparator();
        cout << "Appuyez sur ENTRÉE pour démarrer la première allocation...\n";
        cin.get();

        // CYCLE 1
        currentCycle = 1;
        runCycle(unit);
        
        cout << "\n\n";
        Display::printSeparator();
        cout << "Appuyez sur ENTRÉE pour continuer vers le cycle suivant...\n";
        cin.ignore();
        cin.get();

        // CYCLES SUIVANTS
        while(!allProcessesFinished()) {
            currentCycle++;
            this_thread::sleep_for(chrono::milliseconds(cycleDelay));
            runCycle(unit);
        }

        showFinalReport();
    }

private:
    void runCycle(double unit) {
        Display::clearScreen();
        Display::printHeader("🔄 CYCLE " + to_string(currentCycle));

        CycleStats stats;
        stats.cycleNumber = currentCycle;
        stats.activeProcesses = 0;
        stats.totalAllocated = 0;

        logFile << "=== CYCLE " << currentCycle << " ===\n";
        
        if(currentCycle > 1) {
            jsonFile << ",\n";
        }
        jsonFile << "      {\n";
        jsonFile << "        \"cycle\": " << currentCycle << ",\n";
        jsonFile << "        \"allocations\": [\n";

        double totalWeight = 0;
        for(auto &q : queues) totalWeight += q.weight;

        bool firstQueue = true;
        for(auto &q : queues) {
            double quota = (q.weight / totalWeight) * totalResource;
            
            if(!firstQueue) jsonFile << ",\n";
            jsonFile << "          {\n";
            jsonFile << "            \"queue\": \"" << q.name << "\",\n";
            jsonFile << "            \"quota\": " << quota << ",\n";
            jsonFile << "            \"processes\": [\n";
            
            allocateInQueue(q, quota, unit, stats);
            
            jsonFile << "            ]\n";
            jsonFile << "          }";
            firstQueue = false;
        }

        jsonFile << "\n        ]\n";
        jsonFile << "      }";

        Display::printAllocationTable(queues);
        Display::printDetailedGrid(queues, totalResource);

        history.push_back(stats);
        logFile << "\n";
    }

    void allocateInQueue(Queue &q, double quota, double unit, CycleStats& stats) {
        logFile << "\n[" << q.name << "] (" << q.policy << ") - Quota: " << quota << "\n";

        bool firstProcess = true;
        
        if(q.policy == "RR") {
            roundRobin(q, quota, unit, stats, firstProcess);
        } else {
            fifo(q, quota, unit, stats, firstProcess);
        }
    }

    void roundRobin(Queue &q, double quota, double unit, CycleStats& stats, bool& firstProcess) {
        if(q.processes.empty()) return;
        
        int n = q.processes.size();
        int consecutiveSkips = 0;
        
        while(quota > 0 && consecutiveSkips < n) {
            Process &p = q.processes[q.rrIndex];
            
            if(!p.finished) {
                double alloc = min({p.remaining, unit, quota});
                p.remaining -= alloc;
                p.allocated += alloc;
                quota -= alloc;
                q.totalAllocated += alloc;
                stats.totalAllocated += alloc;
                stats.activeProcesses++;
                
                if(p.startCycle == -1) p.startCycle = currentCycle;
                
                logAllocation(q.name, p.name, alloc, firstProcess);
                
                if(p.remaining <= 0) {
                    p.finished = true;
                    p.endCycle = currentCycle;
                    logFile << "    ✅ " << p.name << " TERMINÉ\n";
                }
                consecutiveSkips = 0;
            } else {
                consecutiveSkips++;
            }
            
            q.rrIndex = (q.rrIndex + 1) % n;
        }
    }

    void fifo(Queue &q, double quota, double unit, CycleStats& stats, bool& firstProcess) {
        for(auto &p : q.processes) {
            if(quota <= 0) break;
            if(!p.finished) {
                double alloc = min({p.remaining, unit, quota});
                p.remaining -= alloc;
                p.allocated += alloc;
                quota -= alloc;
                q.totalAllocated += alloc;
                stats.totalAllocated += alloc;
                stats.activeProcesses++;
                
                if(p.startCycle == -1) p.startCycle = currentCycle;
                
                logAllocation(q.name, p.name, alloc, firstProcess);
                
                if(p.remaining <= 0) {
                    p.finished = true;
                    p.endCycle = currentCycle;
                    logFile << "    ✅ " << p.name << " TERMINÉ\n";
                }
            }
        }
    }

    void logAllocation(const string &queue, const string &process, double alloc, bool& firstProcess) {
        logFile << "  ➜ " << process << " reçoit " << fixed << setprecision(2) 
                << alloc << " unités\n";
        
        if(!firstProcess) jsonFile << ",\n";
        jsonFile << "              {\"process\": \"" << process 
                 << "\", \"allocated\": " << alloc << "}";
        firstProcess = false;
    }

    void showFinalReport() {
        Display::clearScreen();
        Display::printHeader("📊 RAPPORT FINAL DE SIMULATION");

        cout << "\n✅ Tous les processus sont terminés!\n";
        cout << "   Nombre total de cycles: " << currentCycle << "\n\n";

        Display::printSeparator();
        cout << "\n📈 STATISTIQUES PAR FILE:\n\n";

        for(const auto& q : queues) {
            cout << "  " << q.name << ":\n";
            cout << "    ├─ Ressources allouées: " << fixed << setprecision(2) 
                 << q.totalAllocated << " unités\n";
            
            int completedCount = 0;
            for(const auto& p : q.processes) {
                if(p.finished) completedCount++;
            }
            cout << "    └─ Processus terminés: " << completedCount << "/" 
                 << q.processes.size() << "\n\n";
        }

        Display::printSeparator();
        cout << "\n📋 DÉTAILS DES PROCESSUS:\n\n";

        for(const auto& q : queues) {
            for(const auto& p : q.processes) {
                cout << "  • " << p.name << " (" << q.name << "):\n";
                cout << "    ├─ Demande initiale: " << p.demand << " unités\n";
                cout << "    ├─ Total alloué: " << p.allocated << " unités\n";
                cout << "    ├─ Début: Cycle " << p.startCycle << "\n";
                cout << "    └─ Fin: Cycle " << p.endCycle 
                     << " (durée: " << (p.endCycle - p.startCycle + 1) << " cycles)\n\n";
            }
        }

        logFile << "\n=== RAPPORT FINAL ===\n";
        logFile << "Nombre de cycles: " << currentCycle << "\n";
        logFile << "Tous les processus terminés avec succès.\n";

        cout << "\n💾 Fichiers générés:\n";
        cout << "  • allocation_log.txt (journal détaillé)\n";
        cout << "  • allocation_data.json (données structurées)\n\n";
    }
};

// ==================== MAIN ====================
int main() {
    cout << "\n";
    cout << "╔════════════════════════════════════════════════════════════════════╗\n";
    cout << "║         SIMULATEUR D'ALLOCATION DE RESSOURCES v2.0                 ║\n";
    cout << "║                   Multi-Queue Scheduler                            ║\n";
    cout << "╚════════════════════════════════════════════════════════════════════╝\n";
    cout << "\n";

    ResourceAllocator allocator(100.0);

    Queue q1 = {"File 1 (VVIP)", {
        {"P1", 50, 50, 1}, 
        {"P2", 30, 30, 1}
    }, 0.5, "RR"};

    Queue q2 = {"File 2 (VIP)", {
        {"P3", 60, 60, 2}, 
        {"P4", 40, 40, 2}
    }, 0.3, "RR"};

    Queue q3 = {"File 3 (CLASSIC)", {
        {"P5", 80, 80, 3}, 
        {"P6", 20, 20, 3}
    }, 0.2, "FIFO"};

    allocator.addQueue(q1);
    allocator.addQueue(q2);
    allocator.addQueue(q3);

    allocator.simulate(10.0, 3000); // quantum=10, délai=3000ms (3 secondes)

    return 0;
}