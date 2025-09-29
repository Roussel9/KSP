# Ninja Virtual Machine (NJVM)

Dies ist meine Implementierung der **Ninja Virtual Machine** im Rahmen der KSP-Übung.  
Das Projekt umfasst die dynamische Verwaltung von **Stack** und **Heap** sowie die Anbindung der externen **BigInt-Bibliothek**.

---

## Features
- Heap und Stack werden beim Programmstart dynamisch mit `malloc()` angelegt
- Größe von Heap und Stack kann über Kommandozeilenparameter angegeben werden:
  - `--heap n` → Heapgröße `n * 1024` Bytes (Standard: 8192 KiB)
  - `--stack n` → Stackgröße `n * 1024` Bytes (Standard: 64 KiB)
- Heap wird in zwei gleich große Hälften geteilt  
- Speicherallokation erfolgt fortlaufend aus einer Heap-Hälfte  
- Bei Überlauf → Ausgabe: `Error: heap overflow`  
- Unterstützung für primitive und zusammengesetzte Objekte  
- Nutzung der BigInt-Bibliothek für arithmetische Operationen

---


