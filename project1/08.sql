SELECT AVG(cp.level)
FROM CatchedPokemon cp
WHERE cp.owner_id IN (SELECT t.id FROM Trainer t WHERE t.name = 'Red');