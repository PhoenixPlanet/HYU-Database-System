SELECT name, level_sum
FROM (
    SELECT t.name AS name, SUM(cp.level) AS level_sum
    FROM Trainer t, CatchedPokemon cp 
    WHERE t.id = cp.owner_id
    GROUP BY t.name
) AS ls
WHERE level_sum = (
    SELECT MAX(ls2.sum_level) FROM (
        SELECT SUM(cp.level) AS sum_level
        FROM Trainer t, CatchedPokemon cp 
        WHERE t.id = cp.owner_id
        GROUP BY t.name
    ) AS ls2
)
ORDER BY name;