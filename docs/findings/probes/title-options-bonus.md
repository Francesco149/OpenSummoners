# probe title-options-bonus

run `title-opts`; accepted 2026-11-13 21:21

check PASS (tools/drive/tasks/title-options-bonus.md)
verify MATCH (+ real device)
filed tests/scenarios/title-options-bonus/trace.jsonl (6577 frames)
bless: PASS — [batch 20261113-212304-corpus: 1 run(s) in 18.3s (-j8) → /mnt/ephemeral/opensummoners/rhost-runs/20261113-212304-corpus] | title-options-bonus                      ok    frames   f=  6697 t=     0 rng=a8494895 sem=9dd7dcc1cd9187eb 501fps complete=1 | CORPUS: 1/1 complete; results in /mnt/ephemeral/opensummoners/rhost-runs/20261113-212304-corpus | blessed expected.json for every ok scenario (volatile.<edition>.txt changes state_sem: re-bless after --calibrate)
hybrid: FAIL —     hook-result replace ent_move_v calls=0 | CORPUS: 1/1 complete; results in /mnt/ephemeral/opensummoners/rhost-runs/20261113-212324-corpus |   REGRESSION title-options-bonus: state_sem 9dd7dcc1cd9187eb→8c2ef9970e94aee3 | CHECK: FAIL (1 scenario(s) differ from expected.json)
standalone: PASS —     hook-result replace ent_move_h calls=0 |     hook-result replace ent_move_v calls=0 | CORPUS: 1/1 complete; results in /mnt/ephemeral/opensummoners/rhost-runs/20261113-212349-corpus | CHECK: PASS (0 scenario(s) differ from expected.json)
