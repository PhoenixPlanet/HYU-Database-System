SELECT p.name
FROM Pokemon p, CatchedPokemon cp 
WHERE p.id = cp.pid AND cp.nickname LIKE "% %"
ORDER BY p.name DESC;