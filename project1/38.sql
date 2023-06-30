SELECT p.name AS name
FROM Pokemon p,
(
    SELECT e1.after_id AS final
    FROM Evolution e1 
    WHERE e1.after_id NOT IN (SELECT e2.before_id FROM Evolution e2)
) AS finalEvolution
WHERE p.id = finalEvolution.final
ORDER BY name;