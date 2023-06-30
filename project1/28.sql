SELECT t.name AS name, AVG(cp.level) AS avg_level
FROM Trainer t, CatchedPokemon cp 
WHERE t.id = cp.owner_id AND cp.pid IN (SELECT p.id FROM Pokemon p WHERE p.type = 'Normal' OR p.type = 'Electric')
GROUP BY name
ORDER BY avg_level;