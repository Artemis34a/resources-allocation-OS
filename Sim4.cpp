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
#include <cmath>

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
    double waitTime = 0.0;
    double basePriority;
};

struct Queue {
    string name;
    vector<Process> processes;
    double weight;
    string policy;
    int rrIndex = 0;
    double totalAllocated = 0.0;
    double quota = 0.0;
    string color;
    string emoji;
};

struct CycleStats {
    int cycleNumber;
    map<string, double> queueAllocations;
    map<string, vector<pair<string, double>>> processAllocations;
    int activeProcesses;
    double totalAllocated;
    double utilization;
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

    static void printBanner() {
        cout << "\033[1;36m"; // Cyan bold
        cout << "\n╔═══════════════════════════════════════════════════════════════════════╗\n";
        cout << "║                                                                       ║\n";
        cout << "║          🚀        SIMULATEUR CAP-PRO-RATA                🚀          ║\n";
        cout << "║                                                                       ║\n";
        cout << "║              Allocation Multi-Niveaux de Ressources                   ║\n";
        cout << "║                  Akamba Biyembe aka Artemis                           ║\n";
        cout << "║                                                                       ║\n";
        cout << "╚═══════════════════════════════════════════════════════════════════════╝\n";
        cout << "\033[0m"; // Reset
    }

    static void printHeader(const string& title, int cycle = -1) {
        cout << "\n\033[1;34m"; // Blue bold
        cout << "╔═══════════════════════════════════════════════════════════════════════╗\n";
        if(cycle >= 0) {
            cout << "║  " << title << " " << cycle << string(58 - title.length() - to_string(cycle).length(), ' ') << "║\n";
        } else {
            cout << "║  " << title << string(68 - title.length(), ' ') << "║\n";
        }
        cout << "╚═══════════════════════════════════════════════════════════════════════╝\n";
        cout << "\033[0m";
    }

    static void printSeparator(char style = '─') {
        cout << "\033[0;90m"; // Dark gray
        for(int i = 0; i < 75; i++) cout << style;
        cout << "\033[0m\n";
    }

    static string getStatusEmoji(bool finished) {
        return finished ? "\033[1;32m✅\033[0m" : "\033[1;33m⏳\033[0m";
    }

    static string getProgressBar(double current, double total, int width = 20) {
        int filled = (int)((current / total) * width);
        string bar = "\033[1;32m[\033[0m";
        for(int i = 0; i < width; i++) {
            if(i < filled) bar += "\033[1;32m█\033[0m";
            else bar += "\033[0;90m░\033[0m";
        }
        bar += "\033[1;32m]\033[0m";
        return bar;
    }

    static void printResourceGrid(double totalResource, const vector<Queue>& queues, int gridWidth = 60) {
        cout << "\n\033[1;35m📊 DISTRIBUTION DES RESSOURCES\033[0m\n";
        cout << "   Total disponible: \033[1;33m" << fixed << setprecision(1) 
             << totalResource << " unités\033[0m\n\n";
        
        double totalWeight = 0;
        for(const auto& q : queues) totalWeight += q.weight;
        
        cout << "   ┌";
        for(int i = 0; i < gridWidth; i++) cout << "─";
        cout << "┐\n   │";

        for(const auto& q : queues) {
            int cells = (int)((q.weight / totalWeight) * gridWidth);
            string colorCode = q.color;
            
            for(int i = 0; i < cells; i++) {
                if(i == cells/2) cout << q.emoji;
                else cout << colorCode << "█\033[0m";
            }
        }
        
        cout << "│\n   └";
        for(int i = 0; i < gridWidth; i++) cout << "─";
        cout << "┘\n\n";

        // Légende améliorée
        cout << "   \033[1;37mLÉGENDE:\033[0m\n";
        for(const auto& q : queues) {
            double quota = (q.weight / totalWeight) * totalResource;
            double percentage = (q.weight / totalWeight) * 100;
            cout << "   " << q.emoji << " " << q.color << q.name << "\033[0m"
                 << " │ Poids: " << fixed << setprecision(2) << q.weight
                 << " │ Quota: " << setprecision(1) << quota << " unités"
                 << " │ Part: " << setprecision(1) << percentage << "%\n";
        }
    }

    static void printAllocationTable(const vector<Queue>& queues) {
        cout << "\n\033[1;36m📋 TABLEAU D'ALLOCATION DÉTAILLÉ\033[0m\n\n";
        
        for(const auto& q : queues) {
            cout << "  " << q.emoji << " " << q.color << "━━━ " << q.name 
                 << " [" << q.policy << "] ━━━\033[0m\n";
            cout << "  ┌────────────┬──────────┬──────────┬──────────┬──────────┬────────┐\n";
            cout << "  │ Processus  │ Demande  │ Restant  │  Alloué  │ Progress │  État  │\n";
            cout << "  ├────────────┼──────────┼──────────┼──────────┼──────────┼────────┤\n";
            
            for(const auto& p : q.processes) {
                cout << "  │ " << setw(10) << left << p.name 
                     << " │ " << setw(8) << right << fixed << setprecision(1) << p.demand
                     << " │ " << setw(8) << p.remaining
                     << " │ " << setw(8) << p.allocated
                     << " │ " << getProgressBar(p.allocated, p.demand, 8)
                     << " │ " << getStatusEmoji(p.finished) << "   │\n";
            }
            cout << "  └────────────┴──────────┴──────────┴──────────┴──────────┴────────┘\n";
            
            // Statistiques de la file
            int completed = 0, total = q.processes.size();
            double totalDemand = 0, totalAllocated = 0;
            for(const auto& p : q.processes) {
                if(p.finished) completed++;
                totalDemand += p.demand;
                totalAllocated += p.allocated;
            }
            
            cout << "  📊 Stats: " << completed << "/" << total << " terminés"
                 << " │ Total alloué: " << fixed << setprecision(1) << totalAllocated 
                 << "/" << totalDemand << " unités\n\n";
        }
    }

    static void printDetailedProgress(const vector<Queue>& queues, double totalResource) {
        cout << "\n\033[1;33m🎯 PROGRESSION DÉTAILLÉE PAR PROCESSUS\033[0m\n\n";
        
        for(const auto& q : queues) {
            cout << "  " << q.emoji << " " << q.color << q.name << "\033[0m\n";
            
            for(const auto& p : q.processes) {
                double progress = (p.allocated / p.demand) * 100;
                cout << "    ├─ " << setw(12) << left << p.name << " ";
                
                // Barre de progression colorée
                int barWidth = 30;
                int filled = (int)((progress / 100.0) * barWidth);
                cout << "[";
                for(int i = 0; i < barWidth; i++) {
                    if(i < filled) {
                        if(progress >= 100) cout << "\033[1;32m█\033[0m";
                        else if(progress >= 66) cout << "\033[1;33m█\033[0m";
                        else if(progress >= 33) cout << "\033[1;36m█\033[0m";
                        else cout << "\033[1;31m█\033[0m";
                    } else {
                        cout << "\033[0;90m░\033[0m";
                    }
                }
                cout << "] " << fixed << setprecision(1) << progress << "%";
                
                if(p.finished) {
                    cout << " \033[1;32m✓ COMPLÉTÉ\033[0m";
                }
                cout << "\n";
            }
            cout << "\n";
        }
    }

    static void printCycleMetrics(int cycle, const CycleStats& stats, double totalResource) {
        cout << "\n\033[1;35m📈 MÉTRIQUES DU CYCLE " << cycle << "\033[0m\n";
        cout << "  ┌─────────────────────────────────────────────┐\n";
        cout << "  │ Processus actifs    : " << setw(18) << right << stats.activeProcesses << "  │\n";
        cout << "  │ Ressources allouées : " << setw(15) << fixed << setprecision(2) 
             << stats.totalAllocated << " unités │\n";
        cout << "  │ Taux d'utilisation  : " << setw(17) << setprecision(1) 
             << stats.utilization << "% │\n";
        cout << "  └─────────────────────────────────────────────┘\n";
    }

    static void printWaitingAnimation(int duration_ms) {
        const string anim[] = {"⠋", "⠙", "⠹", "⠸", "⠼", "⠴", "⠦", "⠧", "⠇", "⠏"};
        int steps = duration_ms / 100;
        
        for(int i = 0; i < steps; i++) {
            cout << "\r  " << anim[i % 10] << " Allocation en cours..." << flush;
            this_thread::sleep_for(chrono::milliseconds(100));
        }
        cout << "\r  ✓ Allocation terminée!            \n";
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
    bool useAging = true;
    double agingFactor = 0.05;

public:
    ResourceAllocator(double totalRes) : totalResource(totalRes) {
        time_t now = time(0);
        tm *ltm = localtime(&now);
        
        logFile.open("allocation_log.txt");
        jsonFile.open("allocation_data.json");
        
        logFile << "═══════════════════════════════════════════════════════\n";
        logFile << "  SIMULATION CAP-PRO-RATA - LOG DÉTAILLÉ\n";
        logFile << "═══════════════════════════════════════════════════════\n";
        logFile << "Date de simulation: " << asctime(ltm);
        logFile << "Ressource totale: " << totalResource << " unités\n";
        logFile << "Aging activé: " << (useAging ? "OUI" : "NON") << "\n\n";
        
        jsonFile << "{\n  \"simulation\": {\n";
        jsonFile << "    \"totalResource\": " << totalResource << ",\n";
        jsonFile << "    \"timestamp\": \"" << asctime(ltm) << "\",\n";
        jsonFile << "    \"cycles\": [\n";
    }

    ~ResourceAllocator() {
        jsonFile << "    ]\n  }\n}\n";
        logFile.close();
        jsonFile.close();
    }

    void addQueue(Queue &q) { 
        queues.push_back(q); 
    }

    void showInitialState() {
        Display::clearScreen();
        Display::printBanner();
        Display::printHeader("🔧 CONFIGURATION INITIALE DU SYSTÈME");
        
        cout << "\n\033[1;37m📦 FILES D'ATTENTE CONFIGURÉES:\033[0m\n\n";
        
        double totalWeight = 0;
        for(const auto& q : queues) totalWeight += q.weight;
        
        int fileNum = 1;
        for(const auto& q : queues) {
            double quota = (q.weight / totalWeight) * totalResource;
            double percentage = (q.weight / totalWeight) * 100;
            
            cout << "  " << q.emoji << " \033[1;37mFile " << fileNum++ << ": " 
                 << q.color << q.name << "\033[0m\n";
            cout << "     ├─ Politique       : " << q.policy << "\n";
            cout << "     ├─ Poids           : " << fixed << setprecision(3) << q.weight 
                 << " (" << setprecision(1) << percentage << "%)\n";
            cout << "     ├─ Quota théorique : " << setprecision(1) << quota << " unités\n";
            cout << "     └─ Processus       : " << q.processes.size() << "\n";
            
            for(const auto& p : q.processes) {
                cout << "         • " << p.name << " (demande: " 
                     << setprecision(1) << p.demand << " unités)\n";
            }
            cout << "\n";
        }

        Display::printResourceGrid(totalResource, queues);
        Display::printAllocationTable(queues);

        logFile << "═══ CONFIGURATION INITIALE ═══\n";
        for(const auto& q : queues) {
            logFile << q.name << " (" << q.policy << "): " 
                   << q.processes.size() << " processus\n";
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

    void simulate(double unit, int cycleDelay = 2000, bool autoMode = false) {
        showInitialState();
        
        if(!autoMode) {
            cout << "\n\n";
            Display::printSeparator('═');
            cout << "\033[1;33m⚡ Appuyez sur ENTRÉE pour démarrer la simulation...\033[0m\n";
            Display::printSeparator('═');
            cin.get();
        } else {
            cout << "\n\033[1;32m🚀 Mode automatique activé - démarrage dans 2 secondes...\033[0m\n";
            this_thread::sleep_for(chrono::milliseconds(2000));
        }

        // CYCLES D'ALLOCATION
        while(!allProcessesFinished()) {
            currentCycle++;
            this_thread::sleep_for(chrono::milliseconds(cycleDelay));
            runCycle(unit);
            
            if(!autoMode && currentCycle == 1) {
                cout << "\n\n";
                Display::printSeparator('═');
                cout << "\033[1;33m⏭️  Appuyez sur ENTRÉE pour continuer...\033[0m\n";
                Display::printSeparator('═');
                cin.ignore();
                cin.get();
            }
        }

        showFinalReport();
    }

private:
    void runCycle(double unit) {
        Display::clearScreen();
        Display::printBanner();
        Display::printHeader("🔄 CYCLE D'ALLOCATION", currentCycle);

        CycleStats stats;
        stats.cycleNumber = currentCycle;
        stats.activeProcesses = 0;
        stats.totalAllocated = 0;

        logFile << "═══════════════════════════════════════\n";
        logFile << "CYCLE " << currentCycle << "\n";
        logFile << "═══════════════════════════════════════\n";
        
        if(currentCycle > 1) jsonFile << ",\n";
        jsonFile << "      {\n        \"cycle\": " << currentCycle << ",\n";
        jsonFile << "        \"allocations\": [\n";

        double totalWeight = 0;
        for(auto &q : queues) {
            double demandSum = 0;
            for(auto &p : q.processes) {
                if(!p.finished) demandSum += p.remaining;
            }
            totalWeight += q.weight * demandSum;
        }

        bool firstQueue = true;
        for(auto &q : queues) {
            double demandSum = 0;
            for(auto &p : q.processes) {
                if(!p.finished) demandSum += p.remaining;
            }
            
            double quota = totalWeight > 0 ? 
                          (q.weight * demandSum / totalWeight) * totalResource : 0;
            q.quota = quota;
            
            if(!firstQueue) jsonFile << ",\n";
            jsonFile << "          {\n            \"queue\": \"" << q.name << "\",\n";
            jsonFile << "            \"quota\": " << quota << ",\n";
            jsonFile << "            \"processes\": [\n";
            
            allocateInQueue(q, quota, unit, stats);
            
            jsonFile << "            ]\n          }";
            firstQueue = false;
        }

        jsonFile << "\n        ]\n      }";

        // Calcul utilisation
        stats.utilization = (stats.totalAllocated / totalResource) * 100;

        Display::printCycleMetrics(currentCycle, stats, totalResource);
        Display::printResourceGrid(totalResource, queues);
        Display::printAllocationTable(queues);
        Display::printDetailedProgress(queues, totalResource);

        history.push_back(stats);
        logFile << "\n";
    }

    void allocateInQueue(Queue &q, double quota, double unit, CycleStats& stats) {
        logFile << "\n[" << q.name << "] Policy: " << q.policy 
               << " | Quota: " << fixed << setprecision(2) << quota << "\n";

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
                // Aging
                if(useAging && p.allocated == 0 && currentCycle > 1) {
                    p.waitTime += 1.0;
                    p.priority = max(1, (int)(p.basePriority * (1 + agingFactor * p.waitTime)));
                }
                
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
                    logFile << "    ✅ " << p.name << " TERMINÉ (durée: " 
                           << (p.endCycle - p.startCycle + 1) << " cycles)\n";
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
                    logFile << "    ✅ " << p.name << " TERMINÉ (durée: " 
                           << (p.endCycle - p.startCycle + 1) << " cycles)\n";
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
        Display::printBanner();
        Display::printHeader("🏆 RAPPORT FINAL DE SIMULATION");

        cout << "\n\033[1;32m✅ Simulation terminée avec succès!\033[0m\n";
        cout << "   📊 Cycles totaux: \033[1;33m" << currentCycle << "\033[0m\n\n";

        Display::printSeparator('═');
        cout << "\n\033[1;36m📈 STATISTIQUES GLOBALES PAR FILE\033[0m\n\n";

        for(const auto& q : queues) {
            cout << "  " << q.emoji << " " << q.color << q.name << "\033[0m\n";
            cout << "     ├─ Ressources allouées : " << fixed << setprecision(2) 
                 << q.totalAllocated << " unités\n";
            
            int completedCount = 0;
            double avgDuration = 0;
            for(const auto& p : q.processes) {
                if(p.finished) {
                    completedCount++;
                    avgDuration += (p.endCycle - p.startCycle + 1);
                }
            }
            avgDuration /= max(1, completedCount);
            
            cout << "     ├─ Processus terminés  : " << completedCount << "/" 
                 << q.processes.size() << "\n";
            cout << "     └─ Durée moyenne       : " << fixed << setprecision(1) 
                 << avgDuration << " cycles\n\n";
        }

        Display::printSeparator('═');
        cout << "\n\033[1;35m🔍 DÉTAILS PAR PROCESSUS\033[0m\n\n";

        for(const auto& q : queues) {
            cout << "  " << q.color << "▸ " << q.name << "\033[0m\n";
            for(const auto& p : q.processes) {
                cout << "    • " << setw(12) << left << p.name 
                     << " │ Demande: " << setw(6) << right << fixed << setprecision(1) << p.demand 
                     << " │ Alloué: " << setw(6) << p.allocated 
                     << " │ Début: C" << setw(2) << p.startCycle 
                     << " │ Fin: C" << setw(2) << p.endCycle 
                     << " │ Durée: " << (p.endCycle - p.startCycle + 1) << " cycles\n";
            }
            cout << "\n";
        }

        Display::printSeparator('═');
        cout << "\n\033[1;33m💾 FICHIERS GÉNÉRÉS\033[0m\n";
        cout << "  ✓ allocation_log.txt  (journal détaillé)\n";
        cout << "  ✓ allocation_data.json (données structurées)\n\n";

        logFile << "\n═══════════════════════════════════════\n";
        logFile << "RESULTAT FINAL\n";
        logFile << "═══════════════════════════════════════\n";
        logFile << "Cycles totaux: " << currentCycle << "\n";
        logFile << "Tous les processus terminés avec succès.\n";
    }
};

// ==================== MAIN ====================
int main() {
    Display::clearScreen();
    Display::printBanner();
    
    cout << "\n\033[1;37m🔧 Configuration du système...\033[0m\n\n";

    ResourceAllocator allocator(100.0);

    // Configuration des files avec couleurs et emojis
    Queue q1 = {
        "File 1 (VVIP)", 
        {
            {"P1", 50, 50, 1, false, 0.0, -1, -1, 0.0, 1}, 
            {"P2", 30, 30, 1, false, 0.0, -1, -1, 0.0, 1}
        }, 
        0.5, 
        "RR",
        0,
        0.0,
        0.0,
        "\033[1;31m",  // Rouge
        "🔴"
    };

    Queue q2 = {
        "File 2 (VIP)", 
        {
            {"P3", 60, 60, 2, false, 0.0, -1, -1, 0.0, 2}, 
            {"P4", 40, 40, 2, false, 0.0, -1, -1, 0.0, 2}
        }, 
        0.3, 
        "RR",
        0,
        0.0,
        0.0,
        "\033[1;33m",  // Jaune
        "🟠"
    };

    Queue q3 = {
        "File 3 (CLASSIC)", 
        {
            {"P5", 80, 80, 3, false, 0.0, -1, -1, 0.0, 3}, 
            {"P6", 20, 20, 3, false, 0.0, -1, -1, 0.0, 3}
        }, 
        0.2, 
        "FIFO",
        0,
        0.0,
        0.0,
        "\033[1;32m",  // Vert
        "🟢"
    };

    allocator.addQueue(q1);
    allocator.addQueue(q2);
    allocator.addQueue(q3);

    cout << "  ✓ 3 files configurées\n";
    cout << "  ✓ 6 processus initialisés\n";
    cout << "  ✓ Ressource totale: 100.0 unités\n\n";

    // Lancement de la simulation
    allocator.simulate(10.0, 2000, false); // quantum=10, délai=2s, mode manuel

    return 0;
}
