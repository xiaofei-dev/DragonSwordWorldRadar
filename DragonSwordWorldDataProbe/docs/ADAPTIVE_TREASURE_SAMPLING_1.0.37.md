# Adaptive Treasure Sampling 1.0.37

## Confirmed transition

The 1.0.36 runtime captured Treasure UID `10537365938623186743` (`DT_Monster_G2_10711`) as present with an XY match distance of `0.58021233125807` and a Z difference of `52.676529745331`. The Actor then disappeared and reached `opened_after_observed_presence` after the time and sample gates. This is confirmed runtime evidence, not yet save-database ground truth.

## Performance regression

Enumerating all 11 exact TreasureBox classes once per second caused severe game stutter. The static nearest-neighbour work was small in the observed run; repeated UE4SS `FindAllOf` calls were the dominant avoidable operation.

## Adaptive policy

- If the player is within 25 metres of catalog points, enumerate only the exact classes required by those points.
- If no catalog point is nearby or the player is not ready, enumerate one exact class per pass in rotation.
- Retain class results for at most four samples as diagnostic context.
- Continue writing Actor-to-static identity evidence and the presence-to-absence state chain.

This normally reduces Treasure enumeration from 11 `FindAllOf` calls per second to one call per second.
