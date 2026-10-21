===============================================================================
  AI ENGINE — Instal·lació al teu projecte de Unity
===============================================================================

Aquest paquet conté els arxius del motor que cal instal·lar UNA SOLA
VEGADA per projecte. No cal tornar-lo a descarregar cada cop que
exportes un comportament nou des de l'editor.


-------------------------------------------------------------------------------
  1. ON VA CADA FITXER
-------------------------------------------------------------------------------

  AIEnginePlugin.dll  ->  Assets/Plugins/
  AIEngineTypes.cs    ->  Assets/Scripts/

Si les carpetes no existeixen, crea-les. Unity reconeix "Plugins" com a
carpeta especial per a biblioteques natives.


-------------------------------------------------------------------------------
  2. QUÈ EXPORTA L'EDITOR PER A CADA COMPORTAMENT
-------------------------------------------------------------------------------

Cada cop que exportes un cervell des de l'editor obtens dos fitxers que
comparteixen el nom que hagis triat. Per exemple, per a un cervell
anomenat "Guardian":

  Guardian.bin             ->  Assets/StreamingAssets/
  AIBrainBase_Guardian.cs  ->  Assets/Scripts/

El fitxer .bin conté la definició del comportament que interpreta el
motor. El fitxer .cs conté la classe abstracta de la qual has d'heretar.


-------------------------------------------------------------------------------
  3. COM CONNECTAR-HO AMB EL TEU JOC
-------------------------------------------------------------------------------

Crea una classe que hereti de la classe abstracta generada i implementa
els mètodes que et demana. Aquests mètodes són els que has definit
visualment a l'editor: un per cada acció i un per cada variable.

Exemple:

    using UnityEngine;

    public class GuardianNPC : AIBrainBase_Guardian
    {
        public float speed = 3.5f;
        public Transform player;   // Assigna'l a l'inspector.

        protected override NodeState OnAction_Chase()
        {
            // La teva lògica de moviment.
            // Retorna Running mentre l'acció encara s'està duent a terme,
            // Success quan s'ha completat, o Failure si no s'ha pogut fer.
            transform.position = Vector3.MoveTowards(
                transform.position, player.position, speed * Time.deltaTime);

            return Vector3.Distance(transform.position, player.position) < 1f
                ? NodeState.Success
                : NodeState.Running;
        }

        protected override NodeState OnAction_Patrol()
        {
            // ...
            return NodeState.Running;
        }

        protected override bool OnGetBool_playerVisible()
        {
            // Retorna l'estat real del món del joc.
            return player != null && Vector3.Distance(transform.position,
                                                      player.position) < 10f;
        }
    }

Afegeix aquest component al GameObject de l'NPC i, a l'inspector, escriu
el nom del fitxer .bin al camp "Brain File Name" (per exemple,
"Guardian.bin").


-------------------------------------------------------------------------------
  4. NOTES IMPORTANTS
-------------------------------------------------------------------------------

* Els mètodes d'acció retornen NodeState. Fes servir Running per a
  accions que necessiten diversos fotogrames (desplaçaments, animacions):
  el motor no donarà l'acció per acabada fins que retornis Success o
  Failure.

* Per llegir els paràmetres que has definit per a una acció a l'editor,
  crida GetIntParam, GetFloatParam, GetBoolParam o GetStringParam dins
  del mètode OnAction_* corresponent, per exemple:

      float speed = GetFloatParam("speed", 3.5f);

  El segon argument es retorna si el paràmetre no existeix o és d'un
  altre tipus. Aquests mètodes només funcionen dins d'un mètode OnAction_*.

* Les variables es consulten només quan el comportament les necessita
  en aquell instant, no a cada fotograma. Per això és segur fer-hi
  càlculs moderats, tot i que convé mantenir-los lleugers.

* Els noms de variable distingeixen majúscules i minúscules. Si a
  l'editor has escrit "health", el mètode generat serà
  OnGetInt_health().

* Si reexportes un cervell després de modificar-lo a l'editor, el fitxer
  .cs es regenera: no el modifiquis directament, perquè perdries els
  canvis. Tota la teva lògica ha d'anar a la classe filla.

* La classe generada ja fa servir Awake, Start, Update i OnDestroy.
  Si la teva classe també els necessita, sobreescriu-los i crida el
  mètode base (per exemple, base.Start()).

* Si canvies el plugin natiu, tanca Unity abans de substituir la DLL.
  Unity la manté carregada mentre l'editor està obert i no pots
  sobreescriure-la.


-------------------------------------------------------------------------------
  5. RESOLUCIÓ DE PROBLEMES
-------------------------------------------------------------------------------

"DllNotFoundException: AIEnginePlugin"
    La DLL no és a Assets/Plugins/, o bé no correspon a l'arquitectura
    del teu editor (64 bits). Selecciona-la al Project i revisa'n la
    configuració a l'inspector.

"The engine could not load the brain file"
    La ruta no és correcta, el fitxer .bin no és a
    Assets/StreamingAssets/ o el fitxer no és un cervell vàlid. Comprova que el camp "Brain File Name"
    coincideix exactament amb el nom del fitxer, extensió inclosa.

"The type or namespace name 'NodeState' could not be found"
    Falta AIEngineTypes.cs al projecte, o bé és fora de l'assembly on
    tens la resta de codi.

L'NPC no fa res i no hi ha cap error
    Comprova a la consola que aparegui el missatge de càrrega correcta
    del cervell. Si hi és, revisa que les variables que retornes
    permetin que alguna branca del comportament s'arribi a executar.
