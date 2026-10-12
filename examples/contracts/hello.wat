(module
  (memory (export "memory") 1)
  (data (i32.const 0) "hello from wasm")
  (func (export "call") (param $p i32) (param $l i32) (result i32)
    i32.const 42
  )
)
