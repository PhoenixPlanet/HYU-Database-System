SELECT Pokemon.name 
FROM(
  SELECT p_type
  FROM (SELECT p.type AS p_type, COUNT(*) AS type_count FROM Pokemon p GROUP BY p.type) AS TypeCount
  WHERE type_count = (SELECT MAX(tc.type_count) FROM (SELECT COUNT(*) AS type_count FROM Pokemon p GROUP BY p.type) AS tc) 
      OR type_count = (SELECT MAX(tc2.type_count) FROM (SELECT COUNT(*) AS type_count FROM Pokemon p GROUP BY p.type) AS tc2 WHERE tc2.type_count <>
                          (SELECT MAX(tc3.type_count) FROM (SELECT COUNT(*) AS type_count FROM Pokemon p GROUP BY p.type) AS tc3))) AS MostType
  , Pokemon
WHERE p_type = Pokemon.type
ORDER BY Pokemon.name;