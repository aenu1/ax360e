test_frsqrtex_1:
  #_ REGISTER_IN f1 4.0
  frsqrte. f3, f1
  blr
  #_ REGISTER_OUT f3 0.48535156250000000
  #_ REGISTER_OUT f1 4.0

test_frsqrtex_2:
  #_ REGISTER_IN f1 16.0
  frsqrte. f3, f1
  blr
  #_ REGISTER_OUT f3 0.24267578125000000
  #_ REGISTER_OUT f1 16.0

test_frsqrtex_3:
  #_ REGISTER_IN f1 1.0
  frsqrte. f3, f1
  blr
  #_ REGISTER_OUT f3 0.97070312500000000
  #_ REGISTER_OUT f1 1.0

test_frsqrtex_4:
  #_ REGISTER_IN f1 9.0
  frsqrte. f3, f1
  blr
  #_ REGISTER_OUT f3 0.32421875000000000
  #_ REGISTER_OUT f1 9.0
