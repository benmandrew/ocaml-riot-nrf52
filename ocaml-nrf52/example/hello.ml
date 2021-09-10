
(* option utilities *)
let optmin x y =
  match x,y with
  | None,a | a,None -> a
  | Some x, Some y-> Some (min x y)

let optsucc = function
  | Some x -> Some (x+1)
  | None -> None

let rec fold_left f acc l =
  match l with
  | [] -> acc
  | x::s -> fold_left f (f acc x) s

(* Change-making problem*)
let change_make money_system amount =
  let rec loop n =
    let onepiece acc piece =
      match n - piece with
      | 0 -> (*problem solved with one coin*) 
             Some 1
      | x -> if x < 0 then 
               (*we don't reach 0, we discard this solution*)
               None
             else
               (*we search the smallest path different to None with the remaining pieces*) 
               optmin (optsucc (loop x)) acc
    in
    (*we call onepiece forall the pieces*)
    fold_left onepiece None money_system
  in loop amount

let () =
  print_string "Change is:\n";
  flush stdout;
  match change_make [10;7;5] 49 with
  | None -> print_string "None";
  | Some v -> print_int v;
  print_char '\n';
  flush stdout
  ;;

(* let () =
  Printf.printf "Hello from OCaml!" *)
