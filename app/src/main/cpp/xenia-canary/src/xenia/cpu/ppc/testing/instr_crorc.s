test_crorc_1:
  #_ REGISTER_IN r3 1
  #_ REGISTER_IN r4 2
  #_ REGISTER_IN r5 3
  #_ REGISTER_IN r6 4
  cmpw cr0, r3, r4
  cmpw cr1, r5, r6
  crorc 2, 0, 4
  mfcr r12
  blr
  #_ REGISTER_OUT r3 1
  #_ REGISTER_OUT r4 2
  #_ REGISTER_OUT r5 3
  #_ REGISTER_OUT r6 4
  #_ REGISTER_OUT r12 0xA8000000

test_crorc_1_constant:
  li r3, 1
  li r4, 2
  li r5, 3
  li r6, 4
  cmpw cr0, r3, r4
  cmpw cr1, r5, r6
  crorc 2, 0, 4
  mfcr r12
  blr
  #_ REGISTER_OUT r3 1
  #_ REGISTER_OUT r4 2
  #_ REGISTER_OUT r5 3
  #_ REGISTER_OUT r6 4
  #_ REGISTER_OUT r12 0xA8000000

test_crorc_2:
  #_ REGISTER_IN r3 2
  #_ REGISTER_IN r4 1
  #_ REGISTER_IN r5 4
  #_ REGISTER_IN r6 3
  cmpw cr0, r3, r4
  cmpw cr1, r5, r6
  crorc 2, 1, 5
  mfcr r12
  blr
  #_ REGISTER_OUT r3 2
  #_ REGISTER_OUT r4 1
  #_ REGISTER_OUT r5 4
  #_ REGISTER_OUT r6 3
  #_ REGISTER_OUT r12 0x64000000

test_crorc_2_constant:
  li r3, 2
  li r4, 1
  li r5, 4
  li r6, 3
  cmpw cr0, r3, r4
  cmpw cr1, r5, r6
  crorc 2, 1, 5
  mfcr r12
  blr
  #_ REGISTER_OUT r3 2
  #_ REGISTER_OUT r4 1
  #_ REGISTER_OUT r5 4
  #_ REGISTER_OUT r6 3
  #_ REGISTER_OUT r12 0x64000000

test_crorc_3:
  #_ REGISTER_IN r3 1
  #_ REGISTER_IN r4 1
  #_ REGISTER_IN r5 2
  #_ REGISTER_IN r6 2
  cmpw cr0, r3, r4
  cmpw cr1, r5, r6
  crorc 2, 2, 6
  mfcr r12
  blr
  #_ REGISTER_OUT r3 1
  #_ REGISTER_OUT r4 1
  #_ REGISTER_OUT r5 2
  #_ REGISTER_OUT r6 2
  #_ REGISTER_OUT r12 0x22000000

test_crorc_3_constant:
  li r3, 1
  li r4, 1
  li r5, 2
  li r6, 2
  cmpw cr0, r3, r4
  cmpw cr1, r5, r6
  crorc 2, 2, 6
  mfcr r12
  blr
  #_ REGISTER_OUT r3 1
  #_ REGISTER_OUT r4 1
  #_ REGISTER_OUT r5 2
  #_ REGISTER_OUT r6 2
  #_ REGISTER_OUT r12 0x22000000

test_crorc_4:
  #_ REGISTER_IN r3 5
  #_ REGISTER_IN r4 10
  #_ REGISTER_IN r5 15
  #_ REGISTER_IN r6 15
  cmpw cr2, r3, r4
  cmpw cr3, r5, r6
  crorc 10, 8, 14
  mfcr r12
  blr
  #_ REGISTER_OUT r3 5
  #_ REGISTER_OUT r4 10
  #_ REGISTER_OUT r5 15
  #_ REGISTER_OUT r6 15
  #_ REGISTER_OUT r12 0x00A20000

test_crorc_4_constant:
  li r3, 5
  li r4, 10
  li r5, 15
  li r6, 15
  cmpw cr2, r3, r4
  cmpw cr3, r5, r6
  crorc 10, 8, 14
  mfcr r12
  blr
  #_ REGISTER_OUT r3 5
  #_ REGISTER_OUT r4 10
  #_ REGISTER_OUT r5 15
  #_ REGISTER_OUT r6 15
  #_ REGISTER_OUT r12 0x00A20000

test_crorc_5:
  #_ REGISTER_IN r3 100
  #_ REGISTER_IN r4 100
  #_ REGISTER_IN r5 50
  #_ REGISTER_IN r6 25
  cmpw cr4, r3, r4
  cmpw cr5, r5, r6
  crorc 18, 18, 21
  mfcr r12
  blr
  #_ REGISTER_OUT r3 100
  #_ REGISTER_OUT r4 100
  #_ REGISTER_OUT r5 50
  #_ REGISTER_OUT r6 25
  #_ REGISTER_OUT r12 0x00002400

test_crorc_5_constant:
  li r3, 100
  li r4, 100
  li r5, 50
  li r6, 25
  cmpw cr4, r3, r4
  cmpw cr5, r5, r6
  crorc 18, 18, 21
  mfcr r12
  blr
  #_ REGISTER_OUT r3 100
  #_ REGISTER_OUT r4 100
  #_ REGISTER_OUT r5 50
  #_ REGISTER_OUT r6 25
  #_ REGISTER_OUT r12 0x00002400
