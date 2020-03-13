let caml_msr_version = 1

type leaf_t = int64 array
type msrs_t = int64 array

type cpu_policy = {
  leaves      : leaf_t;
  msrs        : msrs_t;
}

type _policy = {
  leaves      : string;
  msrs        : string;
  version     : int;
}


let i2b i =
  match Base64.encode (Int64.to_string i) with
  | Ok x -> x
  | _ -> failwith ("Failed to Base64 Encode: " ^ Int64.to_string i)

let int_array_to_string a =
  a |> Array.map i2b |> Array.to_list |> String.concat ","

let string_to_int_array str =
  let decode e = match Base64.decode e with
    | Ok str    -> Int64.of_string str
    | _         -> failwith ("Failed to Base64 Decode: " ^ e)
  in
  List.map decode (String.split_on_char ',' str) |> Array.of_list

let compress s = s
let decompress s = s

let serialise (p:cpu_policy) =
  {
    leaves  = p.leaves |> int_array_to_string |> compress;
    msrs    = p.msrs   |> int_array_to_string |> compress;
    version = caml_msr_version;
  }

let deserialise (s:_policy) =
  {
    leaves = s.leaves |> decompress |> string_to_int_array;
    msrs   = s.msrs   |> decompress |> string_to_int_array;
  }


