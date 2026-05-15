test_fctidx_1:
  #_ REGISTER_IN f1 3.7
  fctid. f3, f1
  blr
  #_ REGISTER_OUT f3 0x0000000000000004
  #_ REGISTER_OUT f1 3.7

test_fctidx_2:
  #_ REGISTER_IN f1 -2.3
  fctid. f3, f1
  blr
  #_ REGISTER_OUT f3 0xFFFFFFFFFFFFFFFE
  #_ REGISTER_OUT f1 -2.3

test_fctidx_3:
  #_ REGISTER_IN f1 100.9
  fctid. f3, f1
  blr
  #_ REGISTER_OUT f3 0x0000000000000065
  #_ REGISTER_OUT f1 100.9

test_fctidx_4:
  #_ REGISTER_IN f1 0.0
  fctid. f3, f1
  blr
  #_ REGISTER_OUT f3 0x0000000000000000
  #_ REGISTER_OUT f1 0.0
