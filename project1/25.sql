SELECT DISTINCT p.name
FROM Pokemon p
WHERE p.id IN (SELECT cp.pid FROM CatchedPokemon cp, Trainer t WHERE cp.owner_id = t.id AND t.hometown = 'Sangnok City')
    AND p.id IN (SELECT cp2.pid FROM CatchedPokemon cp2, Trainer t2 WHERE cp2.owner_id = t2.id AND t2.hometown = 'Brown City')
ORDER BY p.name;