SELECT cp.nickname
FROM CatchedPokemon cp
WHERE cp.level >= 50 AND cp.owner_id >= 6
ORDER BY cp.nickname;