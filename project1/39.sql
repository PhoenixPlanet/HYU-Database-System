SELECT t.name AS name
FROM Trainer t, CatchedPokemon cp 
WHERE t.id = cp.owner_id
GROUP BY t.id, cp.pid
HAVING COUNT(cp.id) >= 2
ORDER BY name;
