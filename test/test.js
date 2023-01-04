/* eslint-disable */
const { spec } = require('pactum')
const { like, eachLike } = require('pactum-matchers')

it('should get a response with table not exist', async () => {
  await spec()
    .get('/rs/table_name_not_exist_in_db')
    .expectJsonMatch({"status": 701, "message": like("Error")})
});