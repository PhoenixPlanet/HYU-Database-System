SELECT t.name, COUNT(cp.id) AS pokemon_count
FROM Trainer t
LEFT OUTER JOIN CatchedPokemon cp ON t.id = cp.owner_id
WHERE t.id IN (SELECT leader_id FROM Gym)
GROUP BY t.name
ORDER BY t.name;
