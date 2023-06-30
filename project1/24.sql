SELECT c.name, AVG(cp.level) AS avg_level
FROM (Trainer t
INNER JOIN CatchedPokemon cp ON t.id = cp.owner_id)
RIGHT OUTER JOIN City c ON c.name = t.hometown
GROUP BY c.name
ORDER BY avg_level;