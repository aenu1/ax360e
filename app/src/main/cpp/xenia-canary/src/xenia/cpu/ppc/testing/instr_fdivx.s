test_fdivx_1:
  #_ REGISTER_IN f1 6.0
  #_ REGISTER_IN f2 2.0
  fdiv. f3, f1, f2
  blr
  #_ REGISTER_OUT f3 3.0
  #_ REGISTER_OUT f1 6.0
  #_ REGISTER_OUT f2 2.0

test_fdivx_2:
  #_ REGISTER_IN f1 1.0
  #_ REGISTER_IN f2 -1.0
  fdiv. f3, f1, f2
  blr
  #_ REGISTER_OUT f3 -1.0
  #_ REGISTER_OUT f1 1.0
  #_ REGISTER_OUT f2 -1.0

test_fdivx_3:
  #_ REGISTER_IN f1 2.0
  #_ REGISTER_IN f2 2.0
  fdiv. f3, f1, f2
  blr
  #_ REGISTER_OUT f3 1.0
  #_ REGISTER_OUT f1 2.0
  #_ REGISTER_OUT f2 2.0

test_fdivx_4:
  #_ REGISTER_IN f1 4.0
  #_ REGISTER_IN f2 2.0
  fdiv. f3, f1, f2
  blr
  #_ REGISTER_OUT f3 2.0
  #_ REGISTER_OUT f1 4.0
  #_ REGISTER_OUT f2 2.0
