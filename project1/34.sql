SELECT p.name, cp.level, cp.nickname
FROM Pokemon p, CatchedPokemon cp 
WHERE (cp.owner_id IN (SELECT g.leader_id FROM Gym g) AND cp.nickname LIKE "A%") AND p.id = cp.pid
ORDER BY p.name DESC;