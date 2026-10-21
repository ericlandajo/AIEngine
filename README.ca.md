# AIEngine — Motor d'IA modular per a NPC

[English](README.md) | **Català**

Motor d'intel·ligència artificial per a personatges no jugadors (NPC), desacoblat del motor de joc. El comportament es dissenya en un editor visual basat en nodes, com a **arbre de comportament** (BT) o com a **màquina d'estats finits** (FSM). S'exporta a un fitxer binari amb [FlatBuffers](https://flatbuffers.dev/) i el motor l'interpreta i l'executa durant la partida. La integració de referència és amb Unity.

Aquest repositori conté el codi del Treball de Fi de Grau *Disseny i desenvolupament de l'arquitectura modular d'un motor d'IA per a NPC* (Grau en Enginyeria Informàtica, Facultat d'Informàtica de Barcelona, UPC).

## Estructura del repositori

```
.
├── engine/              Nucli del motor (C++)
│   ├── schema/          Esquema de dades (AIBrainTemplate.fbs) i codi C++ generat
│   ├── src/             Motor, nodes propis i API en C per a P/Invoke
│   ├── include/         Dependències de tercers incloses (aitoolkit, FlatBuffers)
│   ├── tests/           Suite de proves (doctest + CTest) i fitxers de prova
│   └── CMakeLists.txt
└── editor/              Editor visual (React + React Flow)
    ├── src/             Codi de l'editor i codi TypeScript generat a partir de l'esquema
    └── public/engine/   Paquet d'instal·lació per a Unity (plugin, tipus compartits i instruccions)
```

L'esquema `engine/schema/AIBrainTemplate.fbs` és l'única font de veritat del format de dades: tant el codi C++ del motor com el codi TypeScript de l'editor es generen a partir d'aquest fitxer.

## Requisits

- **Motor:** CMake 3.20 o superior i un compilador amb suport de C++20 (Visual Studio 2022, GCC o Clang).
- **Editor:** Node.js 20.19 o superior (o 22.12 o superior).
- **Integració:** Unity (provat a Windows).
- **Opcional:** el compilador d'esquemes `flatc` 25.12.19, només si es modifica l'esquema.

## Motor (C++)

Totes les ordres d'aquest apartat s'executen des de la carpeta `engine/`.

### Compilació

```bash
cd engine
cmake -S . -B build
cmake --build build --config Release
```

Es generen tres objectius:

| Objectiu | Descripció |
|---|---|
| `AIEngineCore` | Biblioteca estàtica amb el nucli, independent de qualsevol motor de joc. |
| `AIEnginePlugin` | Biblioteca dinàmica (`AIEnginePlugin.dll` a Windows) que consumeix Unity. |
| `AIEngineDemo` | Programa de consola que carrega un comportament i en simula uns quants fotogrames. |

Per deixar la DLL directament a un projecte de Unity en compilar:

```bash
cmake -S . -B build -DAIENGINE_UNITY_PLUGINS_DIR="C:/ruta/al/projecte/Assets/Plugins"
```

### Proves

```bash
ctest --test-dir build -C Release
```

La suite consta de 155 casos i s'executa sense obrir cap motor de joc. Els fitxers `.bin` de `tests/fixtures/` són cervells exportats des de l'editor o generats per a les proves de rendiment; els `.json` del mateix nom en mostren el contingut en format llegible. Per compilar sense les proves: `-DAIENGINE_BUILD_TESTS=OFF`.

### Programa de consola

Sense arguments, carrega `tests/fixtures/test_brain.bin`. També se li pot indicar un altre cervell:

```bash
./build/Release/AIEngineDemo tests/fixtures/editor_bt_basic.bin
```

(A Linux i macOS l'executable és a `build/AIEngineDemo`.)

## Editor visual

```bash
cd editor
npm ci
npm run dev
```

L'editor s'obre a `http://localhost:5173`. Per generar-ne una versió estàtica: `npm run build` (el resultat queda a `editor/dist/`). L'aplicació s'executa íntegrament al navegador i no necessita cap servidor propi.

Des de l'editor es pot:

- Modelar comportaments com a BT o com a FSM.
- Desar i recuperar el projecte de treball (fitxer `.json` propi de l'editor).
- Exportar un comportament: es descarreguen el fitxer `Nom.bin` i la classe `AIBrainBase_Nom.cs`.
- Descarregar el paquet d'instal·lació del motor per a Unity (botó **Engine Setup**).

## Integració amb Unity

1. Descarrega el paquet d'instal·lació des de l'editor (**Engine Setup**) i copia `AIEnginePlugin.dll` a `Assets/Plugins/` i `AIEngineTypes.cs` a `Assets/Scripts/`. Només cal fer-ho una vegada per projecte.
2. Exporta un comportament des de l'editor. Copia `Nom.bin` a `Assets/StreamingAssets/` i `AIBrainBase_Nom.cs` a `Assets/Scripts/`.
3. Crea una classe que hereti d'`AIBrainBase_Nom` i implementa'n els mètodes abstractes (un per cada acció i un per cada variable definides a l'editor).
4. Afegeix el component a l'NPC i escriu el nom del fitxer `.bin` al camp **Brain File Name** de l'inspector.

Les instruccions completes, amb un exemple i la resolució de problemes habituals, són a [`editor/public/engine/README.txt`](editor/public/engine/README.txt).

## Modificar l'esquema de dades

Si es modifica `engine/schema/AIBrainTemplate.fbs`, cal regenerar el codi de **totes dues** bandes amb la mateixa versió de `flatc` (25.12.19). Des de l'arrel del repositori:

```bash
flatc --cpp -o engine/schema engine/schema/AIBrainTemplate.fbs
flatc --ts  -o editor/src engine/schema/AIBrainTemplate.fbs
```

Després cal recompilar el plugin i substituir `editor/public/engine/AIEnginePlugin.dll` per la versió nova.

## Llicència

Aquest projecte es distribueix sota la llicència MIT. Vegeu el fitxer [LICENSE](LICENSE).

Inclou o fa servir components de tercers, cadascun amb la seva pròpia llicència:

| Component | Ús | Llicència |
|---|---|---|
| [aitoolkit](https://github.com/linkdd/aitoolkit) | Nodes base de l'arbre de comportament (`engine/include/aitoolkit`) | MIT |
| [FlatBuffers](https://github.com/google/flatbuffers) | Serialització (`engine/include/flatbuffers` i paquet npm) | Apache 2.0 |
| [doctest](https://github.com/doctest/doctest) | Marc de proves (es descarrega en compilar) | MIT |
| [React Flow](https://reactflow.dev/) (`@xyflow/react`) | Editor de grafs | MIT |
| [React](https://react.dev/) | Interfície de l'editor | MIT |
| [JSZip](https://stuk.github.io/jszip/) | Paquet d'instal·lació | MIT o GPLv3 (s'usa sota MIT) |