===============================================================================
  AI ENGINE — Installation in your Unity project
===============================================================================

This package contains the engine files that must be installed ONLY ONCE
per project. You do not need to download it again every time you export
a new behaviour from the editor.


-------------------------------------------------------------------------------
  1. WHERE EACH FILE GOES
-------------------------------------------------------------------------------

  AIEnginePlugin.dll  ->  Assets/Plugins/
  AIEngineTypes.cs    ->  Assets/Scripts/

If the folders do not exist, create them. Unity recognises "Plugins" as a
special folder for native libraries.


-------------------------------------------------------------------------------
  2. WHAT THE EDITOR EXPORTS FOR EACH BEHAVIOUR
-------------------------------------------------------------------------------

Every time you export a brain from the editor you get two files that share
the name you chose. For example, for a brain called "Guardian":

  Guardian.bin             ->  Assets/StreamingAssets/
  AIBrainBase_Guardian.cs  ->  Assets/Scripts/

The .bin file contains the behaviour definition that the engine
interprets. The .cs file contains the abstract class you must inherit
from.


-------------------------------------------------------------------------------
  3. HOW TO CONNECT IT TO YOUR GAME
-------------------------------------------------------------------------------

Create a class that inherits from the generated abstract class and
implement the methods it requires. These are the methods you defined
visually in the editor: one for each action and one for each variable.

Example:

    using UnityEngine;

    public class GuardianNPC : AIBrainBase_Guardian
    {
        public float speed = 3.5f;
        public Transform player;   // Assign it in the Inspector.

        protected override NodeState OnAction_Chase()
        {
            // Your movement logic.
            // Return Running while the action is still in progress,
            // Success when it has finished, or Failure if it could not be done.
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
            // Return the actual state of the game world.
            return player != null && Vector3.Distance(transform.position,
                                                      player.position) < 10f;
        }
    }

Add this component to the NPC's GameObject and, in the Inspector, enter
the name of the .bin file in the "Brain File Name" field (for example,
"Guardian.bin").


-------------------------------------------------------------------------------
  4. IMPORTANT NOTES
-------------------------------------------------------------------------------

* Action methods return NodeState. Use Running for actions that need
  several frames (movements, animations): the engine will not consider
  the action finished until you return Success or Failure.

* To read the parameters you defined for an action in the editor, call
  GetIntParam, GetFloatParam, GetBoolParam or GetStringParam inside the
  corresponding OnAction_* method, for example:

      float speed = GetFloatParam("speed", 3.5f);

  The second argument is returned if the parameter does not exist or has
  a different type. These methods only work inside an OnAction_* method.

* Variables are only queried when the behaviour needs them at that
  moment, not on every frame. It is therefore safe to do moderate
  calculations in them, although it is better to keep them light.

* Variable names are case-sensitive. If you wrote "health" in the
  editor, the generated method will be OnGetInt_health().

* If you re-export a brain after modifying it in the editor, the .cs
  file is regenerated: do not edit it directly, or your changes will be
  lost. All your logic must go in the child class.

* The generated class already uses Awake, Start, Update and OnDestroy.
  If your class needs them too, override them and call the base method
  (for example, base.Start()).

* If you replace the native plugin, close Unity before overwriting the
  DLL. Unity keeps it loaded while the editor is open and you will not be
  able to overwrite it.


-------------------------------------------------------------------------------
  5. TROUBLESHOOTING
-------------------------------------------------------------------------------

"DllNotFoundException: AIEnginePlugin"
    The DLL is not in Assets/Plugins/, or it does not match your editor's
    architecture (64-bit). Select it in the Project window and check its
    settings in the Inspector.

"The engine could not load the brain file"
    The path is not correct, the .bin file is not in
    Assets/StreamingAssets/, or the file is not a valid brain. Check that
    the "Brain File Name" field matches the file name exactly, including
    the extension.

"The type or namespace name 'NodeState' could not be found"
    AIEngineTypes.cs is missing from the project, or it is in a different
    assembly from the rest of your code.

The NPC does nothing and there is no error
    Check that the brain-loaded message appears in the console. If it
    does, make sure that the variables you return allow some branch of
    the behaviour to be executed.
