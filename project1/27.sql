SELECT t.name AS name, MAX(cp.level)
FROM Trainer t, CatchedPokemon cp 
WHERE t.id = cp.owner_id
GROUP BY name 
HAVING COUNT(cp.id) >= 4
ORDER BY name;