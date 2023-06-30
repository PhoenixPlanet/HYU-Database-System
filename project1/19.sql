SELECT COUNT(*)
FROM (SELECT DISTINCT p.type FROM Pokemon p WHERE p.id in (SELECT pid FROM CatchedPokemon WHERE owner_id in (SELECT leader_id FROM Gym WHERE city = 'Sangnok City'))) AS SangnokLeaderPokemon;
