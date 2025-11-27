
---

# 🧠 **Simulateur d’Allocation de Ressources v3.0**

### *Multi-Queue Scheduler with RR / FIFO policies, logs, JSON export & animated terminal UI*

---

## 📌 **1. Introduction**

Ce projet implémente un **simulateur d’allocation de ressources** dans un système composé de plusieurs files d’attente (queues), chacune ayant :

* des **processus** avec demande, priorité, état, etc.
* une **politique d’ordonnancement** (RR : Round-Robin ou FIFO)
* un **poids** permettant de définir la part de ressources à allouer à chaque file


---

## 📌 **2. Architecture globale du code**

Le code est organisé en **4 blocs principaux** :

### **2.1 Structures**

Elles modélisent les éléments de la simulation.

```cpp
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
```

#### 🧱 `struct Process`

Décrit un processus avec :

* `demand` : demande initiale
* `remaining` : ressource restante
* `priority` : priorités (1=haute → 3=basse)
* `finished`
* `allocated` : total reçu
* `startCycle`, `endCycle`

#### 🧱 `struct Queue`

Une file d’attente comprenant :

* une liste de processus
* un **poids** → proportion de ressource
* un **policy** (“RR” ou “FIFO”)
* un index pour RR
* un total alloué

#### 🧱 `struct CycleStats`

Historique d’un cycle : allocations par file, par processus…

---

### **2.2 Classe `Display` : Interface utilisateur CLI**

Cette classe gère **tous les affichages avancés**, notamment :

✔ Clear screen
✔ En-têtes stylisés
✔ Grille proportionnelle des ressources
✔ Tableau des processus
✔ Barres de progression ASCII

Elle produit un rendu visuel digne d’un vrai dashboard système :

* `printResourceGrid()`
* `printAllocationTable()`
* `printDetailedGrid()`

---

### **2.3 Classe `ResourceAllocator` : cœur du simulateur**

C’est l’élément principal.
Elle gère :

* les files
* la simulation
* les cycles
* l’export JSON
* le log texte
* le rapport final

#### Méthodes clés :

* `addQueue()`
* `showInitialState()`
* `simulate()`
* `runCycle()`
* `allocateInQueue()`
* `roundRobin()`
* `fifo()`
* `showFinalReport()`

Elle ouvre aussi automatiquement :

* `allocation_log.txt` (journal humainement lisible)
* `allocation_data.json` (compatible DataViz)

---

### **2.4 Fonction `main()`**

Configuration initiale avec :

| File             | Politique | Poids | Processus |
| ---------------- | --------- | ----- | --------- |
| File 1 (VVIP)    | RR        | 0.5   | P1, P2    |
| File 2 (VIP)     | RR        | 0.3   | P3, P4    |
| File 3 (CLASSIC) | FIFO      | 0.2   | P5, P6    |

Puis lancement :

```cpp
allocator.simulate(10.0, 3000);
```

---

## 📌 **3. Fonctionnement de la simulation**

### **3.1 Initialisation**

Le programme affiche :

* la configuration des files
* leur poids
* leur quota
* la grille visuelle
* le tableau des processus

➡️ L’utilisateur appuie sur **Entrée** pour lancer le Cycle 1.

---

## 📌 **4. Algorithme d’allocation**

Pour chaque cycle :

1. Le poids des files → calcul du quota

2. Pour chaque file :

   * si `RR` : Round-Robin
   * si `FIFO` : First-In First-Out

3. On alloue au processus courant :

```
alloc = min(process.remaining, unit, quota)
```

4. Mise à jour :

* `remaining -= alloc`
* `allocated += alloc`
* si `remaining = 0` → terminé

5. Les informations sont exportées en :

   * log texte
   * JSON
   * affichages dynamiques

La simulation s’arrête quand **tous les processus sont terminés**.

---

## 📌 **5. Sorties générées**

### **5.1 Fichier : `allocation_log.txt`**

Journal lisible montrant :

* cycles
* allocations
* files
* complétions de processus

Exemple :

```
[VVIP] RR - Quota: 50
-> P1 reçoit 10 unités
-> P2 reçoit 10 unités
```

---

### **5.2 Fichier : `allocation_data.json`**

Export structuré pour data visualisation :

```json
{
  "simulation": {
    "totalResource": 100,
    "cycles": [
      {
        "cycle": 1,
        "allocations": [
          {
            "queue": "File 1 (VVIP)",
            "quota": 50,
            "processes": [
              {"process":"P1","allocated":10},
              {"process":"P2","allocated":10}
            ]
          }
        ]
      }
    ]
  }
}
```

---

### **5.3 Affichage final**

Le rapport présente :

* ressources totales allouées par file
* temps de complétion de chaque processus
* durée en cycles

Exemple :

```
• P1 (VVIP)
  - Demande: 50
  - Alloué total: 50
  - Cycle début: 1
  - Cycle fin: 5 (durée: 5 cycles)
```

---

## 📌 **6. Comment utiliser / intégrer le simulateur**

### **6.1 Compilation**

Sous Linux / macOS :

```bash
g++ -std=c++17 Sim4.cpp -o allocator
```

Windows : 

```bash
g++ -std=c++17 Sim4.cpp -o allocator.exe
```

---

### **6.2 Exécution**

```bash
./allocator
```
---

### **6.3 Modifier les files**

Méthode :

```cpp
Queue myQueue = {
    "File Custom",
    {
        {"PX", 40, 40, 1},
        {"PY", 20, 20, 2}
    },
    0.4,
    "RR"
};

allocator.addQueue(myQueue);
```

---

### **6.4 Modifier le quantum**

Dans `simulate()` :

```cpp
allocator.simulate(quantum, delay_ms);
```

Exemple :

```cpp
simulate(5.0, 1000);
```

---

## 📌 **7. Points forts de la version **

✔ Architecture propre et extensible
✔ Dashboard terminal complet
✔ Round-Robin & FIFO
✔ Historique complet par cycles
✔ Export JSON prêt pour Grafana, Tableau, PowerBI
✔ Logging exhaustif
✔ Calcul des durées d’exécution par processus
✔ Visualisation de l’occupation des files
✔ Couleurs, emojis, barres, tableaux

---

## 📌 **8. Améliorations possibles**

Si tu veux pousser encore plus loin :

* Support SJF / Priority Scheduling
* Gestion dynamique : arrivée de processus en cours de simulation
* Visualisation web (React + JSON)
* Simulation parallélisée avec threads
* Mode “auto” sans Entrée utilisateur

---

