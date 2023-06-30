SELECT t.name, AVG(p.level)
FROM Trainer t
LEFT OUTER JOIN CatchedPokemon p ON t.id = p.owner_id
WHERE t.id in (SELECT g.leader_id FROM Gym g)
GROUP BY t.name
ORDER BY t.name;