(module
  (import "env" "storage_get" (func $sget (param i32 i32 i32 i32) (result i32)))
  (import "env" "storage_set" (func $sset (param i32 i32 i32 i32) (result i32)))
  (import "env" "emit_event"  (func $emit (param i32 i32)))
  (memory (export "memory") 1)
  (data (i32.const 0) "count")
  ;; scratch for value at offset 32
  (func (export "call") (param $p i32) (param $l i32) (result i32)
    ;; read current
    (local $n i32)
    (local.set $n (call $sget (i32.const 0) (i32.const 5) (i32.const 64) (i32.const 8)))
    (if (i32.lt_s (local.get $n) (i32.const 0))
      (then (local.set $n (i32.const 48)))     ;; '0' ascii
      (else (local.set $n (i32.load8_u (i32.const 64)))))
    ;; increment ascii digit
    (i32.store8 (i32.const 64) (i32.add (local.get $n) (i32.const 1)))
    (drop (call $sset (i32.const 0) (i32.const 5) (i32.const 64) (i32.const 1)))
    (call $emit (i32.const 0) (i32.const 5))
    (local.get $n)
  )
)
