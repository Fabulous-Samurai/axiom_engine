---- MODULE AxiomDaemonQueueFairness ----
EXTENDS Naturals, TLC

(***************************************************************************)
(* AXIOM daemon queue fairness model                                       *)
(* Focus: enqueued requests are eventually processed and completed         *)
(***************************************************************************)

CONSTANT Requests

States == {"Idle", "Enqueued", "Processing", "Done"}

VARIABLES reqState

vars == <<reqState>>

Init ==
    /\ reqState = [r \in Requests |-> "Idle"]

Enqueue(r) ==
    /\ r \in Requests
    /\ reqState[r] = "Idle"
    /\ reqState' = [reqState EXCEPT ![r] = "Enqueued"]

Start(r) ==
    /\ r \in Requests
    /\ reqState[r] = "Enqueued"
    /\ reqState' = [reqState EXCEPT ![r] = "Processing"]

Complete(r) ==
    /\ r \in Requests
    /\ reqState[r] = "Processing"
    /\ reqState' = [reqState EXCEPT ![r] = "Done"]

Next ==
    \/ \E r \in Requests : Enqueue(r)
    \/ \E r \in Requests : Start(r)
    \/ \E r \in Requests : Complete(r)
    \/ /\ \A r \in Requests : reqState[r] = "Done"
       /\ UNCHANGED reqState

Progress ==
    \/ \E r \in Requests : Enqueue(r)
    \/ \E r \in Requests : Start(r)
    \/ \E r \in Requests : Complete(r)

TypeInv ==
    /\ reqState \in [Requests -> States]

NoRevertFromDone ==
    \A r \in Requests : reqState[r] = "Done" => reqState'[r] = "Done"

EventuallyStart ==
    \A r \in Requests : (reqState[r] = "Enqueued") ~> (reqState[r] = "Processing" \/ reqState[r] = "Done")

EventuallyComplete ==
    \A r \in Requests : (reqState[r] = "Processing") ~> (reqState[r] = "Done")

EventuallyAllDone ==
    <> (\A r \in Requests : reqState[r] = "Done")

Spec ==
    /\ Init
    /\ [][Next]_vars
    /\ WF_vars(Progress)
    /\ \A r \in Requests : WF_vars(Start(r))
    /\ \A r \in Requests : WF_vars(Complete(r))

====
