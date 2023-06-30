SELECT COUNT(cp.id)
FROM Pokemon p, CatchedPokemon cp 
WHERE p.id = cp.pid
GROUP BY p.type
ORDER BY p.type;