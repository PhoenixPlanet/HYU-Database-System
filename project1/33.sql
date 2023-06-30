SELECT SUM(cp.level)
FROM CatchedPokemon cp
WHERE cp.owner_id = (SELECT t.id FROM Trainer t WHERE t.name = 'Matis');