SELECT AVG(level)
FROM CatchedPokemon
WHERE owner_id in (SELECT leader_id FROM Gym);