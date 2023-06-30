SELECT DISTINCT t.name
FROM Trainer t
WHERE t.id IN (SELECT owner_id FROM CatchedPokemon WHERE level <= 10)
ORDER BY t.name;