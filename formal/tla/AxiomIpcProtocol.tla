---- MODULE AxiomIpcProtocol ----
EXTENDS Naturals, Sequences, TLC

(***************************************************************************)
(* AXIOM interactive IPC model                                             *)
(* Focus: mode ack ordering and __END__ frame completion discipline        *)
(***************************************************************************)

CONSTANT Commands

Modes == {"algebraic", "linear", "statistics", "symbolic", "units", "plot"}
Phases == {"Idle", "ModeAckPending", "CommandQueued", "ResultPending", "EndPending"}

VARIABLES mode, phase, pendingCommand, frameEnded

vars == <<mode, phase, pendingCommand, frameEnded>>

Init ==
    /\ mode = "algebraic"
    /\ phase = "Idle"
    /\ pendingCommand = ""
    /\ frameEnded = TRUE

ModeSwitch(m) ==
    /\ phase = "Idle"
    /\ m \in Modes
    /\ m # mode
    /\ mode' = m
    /\ phase' = "ModeAckPending"
    /\ pendingCommand' = pendingCommand
    /\ frameEnded' = FALSE

AckMode ==
    /\ phase = "ModeAckPending"
    /\ mode' = mode
    /\ phase' = "Idle"
    /\ pendingCommand' = ""
    /\ frameEnded' = TRUE

SubmitCommand(c) ==
    /\ phase = "Idle"
    /\ c \in Commands
    /\ mode' = mode
    /\ phase' = "CommandQueued"
    /\ pendingCommand' = c
    /\ frameEnded' = FALSE

StartExecution ==
    /\ phase = "CommandQueued"
    /\ mode' = mode
    /\ phase' = "ResultPending"
    /\ pendingCommand' = pendingCommand
    /\ frameEnded' = FALSE

EmitResult ==
    /\ phase = "ResultPending"
    /\ mode' = mode
    /\ phase' = "EndPending"
    /\ pendingCommand' = pendingCommand
    /\ frameEnded' = FALSE

EmitEnd ==
    /\ phase = "EndPending"
    /\ mode' = mode
    /\ phase' = "Idle"
    /\ pendingCommand' = ""
    /\ frameEnded' = TRUE

Next ==
    \/ \E m \in Modes : ModeSwitch(m)
    \/ AckMode
    \/ \E c \in Commands : SubmitCommand(c)
    \/ StartExecution
    \/ EmitResult
    \/ EmitEnd

TypeInv ==
    /\ mode \in Modes
    /\ phase \in Phases
    /\ pendingCommand \in Commands \cup {""}
    /\ frameEnded \in BOOLEAN

NoCommandWithoutModeAck ==
    phase = "ModeAckPending" => pendingCommand = ""

EndOnlyAfterResult ==
    phase = "EndPending" => pendingCommand # ""

EventuallyTerminates ==
    (phase = "EndPending") ~> (phase = "Idle")

Spec ==
    /\ Init
    /\ [][Next]_vars
    /\ WF_vars(AckMode)
    /\ WF_vars(StartExecution)
    /\ WF_vars(EmitResult)
    /\ WF_vars(EmitEnd)

====