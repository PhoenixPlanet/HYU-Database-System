SELECT owner_id, p_count
FROM (
    SELECT owner_id, COUNT(*) AS p_count
    FROM CatchedPokemon
  	GROUP BY owner_id
) AS CatchedCount 
WHERE p_count = (SELECT MAX(count2) FROM (SELECT COUNT(*) AS count2 FROM CatchedPokemon GROUP BY owner_id) AS CP_Count)
ORDER BY owner_id;