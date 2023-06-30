SELECT ml.city AS city, cp.nickname
FROM (Trainer t
INNER JOIN CatchedPokemon cp ON t.id = cp.owner_id)
RIGHT OUTER JOIN
(
    SELECT c2.name AS city, MAX(cp2.level) AS max_level
    FROM City c2
    LEFT OUTER JOIN Trainer t2 ON c2.name = t2.hometown
    LEFT OUTER JOIN CatchedPokemon cp2 ON t2.id = cp2.owner_id
    GROUP BY city
) AS ml ON ml.city = t.hometown
WHERE max_level IS NULL or max_level = level
ORDER BY city;