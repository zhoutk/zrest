/* eslint-disable */
const assert = require('assert')
const { spec } = require('pactum')
const { like, eachLike } = require('pactum-matchers')

it('test /rs/db_init', async () => {
  await spec()
    .post('/rs/db_init')
    .expectJsonMatch({"status": 200})
});

it('should get a response with table not exist', async () => {
  await spec()
    .get('/rs/table_name_not_exist_in_db')
    .expectJsonMatch({"status": 701})
});

it('test /rs/table/id', async () => {
  const response = await spec()
    .get('/rs/table_for_test/a1b2c3d4')
    .expectJsonMatch({"status": 200})

  const dataZero = response.json['data'][0]
  assert.equal(dataZero['name'], 'Kevin 凯文')
  assert.equal(dataZero['age'], 18)
  assert.equal(dataZero['score'], 99.99)
});