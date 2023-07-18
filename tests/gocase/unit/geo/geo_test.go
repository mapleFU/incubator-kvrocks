/*
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */

package geo

import (
	"context"
	"math"
	"math/rand"
	"reflect"
	"sort"
	"strconv"
	"testing"
	"time"

	"github.com/apache/kvrocks/tests/gocase/util"
	"github.com/redis/go-redis/v9"
	"github.com/stretchr/testify/require"
)

func geoDegrad(deg float64) float64 {
	return deg * math.Atan(1) * 8 / 360
}

func geoRandomPoint() (float64, float64) {
	lon := (-180 + rand.Float64()*360)
	lat := (-70 + rand.Float64()*140)
	return lon, lat
}

func geoDistance(lon1d, lat1d, lon2d, lat2d float64) float64 {
	lon1r := geoDegrad(lon1d)
	lat1r := geoDegrad(lat1d)
	lon2r := geoDegrad(lon2d)
	lat2r := geoDegrad(lat2d)
	v := math.Sin((lon2r - lon1r) / 2)
	u := math.Sin((lat2r - lat1r) / 2)
	return 2.0 * 6372797.560856 * math.Asin(math.Sqrt(u*u+math.Cos(lat1r)*math.Cos(lat2r)*v*v))
}

func compareLists(list1, list2 []string) []string {
	vis := make(map[string]int)
	var result []string
	for i := 0; i < len(list1); i++ {
		j := i
		for j+1 < len(list1) && list1[j+1] == list1[i] {
			j++
		}
		vis[list1[i]] += 1
		i = j
	}
	for i := 0; i < len(list2); i++ {
		j := i
		for j+1 < len(list2) && list2[j+1] == list2[i] {
			j++
		}
		vis[list2[i]] += 1
		i = j
	}
	for _, i := range list1 {
		if val, ok := vis[i]; ok && val == 1 {
			result = append(result, i)
		}
	}
	for _, i := range list2 {
		if val, ok := vis[i]; ok && val == 1 {
			result = append(result, i)
		}
	}
	return result
}

func TestGeo(t *testing.T) {
	srv := util.StartServer(t, map[string]string{})
	defer srv.Close()
	ctx := context.Background()
	rdb := srv.NewClient()
	defer func() { require.NoError(t, rdb.Close()) }()
	t.Run("GEOADD create", func(t *testing.T) {
		require.EqualValues(t, 1, rdb.GeoAdd(ctx, "nyc", &redis.GeoLocation{Name: "lic market", Longitude: -73.9454966, Latitude: 40.747533}).Val())
	})

	t.Run("GEOADD update", func(t *testing.T) {
		require.EqualValues(t, 0, rdb.GeoAdd(ctx, "nyc", &redis.GeoLocation{Name: "lic market", Longitude: -73.9454966, Latitude: 40.747533}).Val())
	})

	t.Run("GEOADD invalid coordinates", func(t *testing.T) {
		require.ErrorContains(t, rdb.Do(ctx, "geoadd", "nyc", -73.9454966, 40.747533, "lic market", "foo", "bar", "luck market").Err(), "valid")
	})

	t.Run("GEOADD multi add", func(t *testing.T) {
		require.EqualValues(t, 6, rdb.GeoAdd(ctx, "nyc", &redis.GeoLocation{Name: "central park n/q/r", Longitude: -73.9733487, Latitude: 40.7648057},
			&redis.GeoLocation{Name: "union square", Longitude: -73.9903085, Latitude: 40.7362513},
			&redis.GeoLocation{Name: "wtc one", Longitude: -74.0131604, Latitude: 40.7126674},
			&redis.GeoLocation{Name: "jfk", Longitude: -73.7858139, Latitude: 40.6428986},
			&redis.GeoLocation{Name: "q4", Longitude: -73.9375699, Latitude: 40.7498929},
			&redis.GeoLocation{Name: "4545", Longitude: -73.9564142, Latitude: 40.7480973}).Val())
	})

	t.Run("Check geoset values", func(t *testing.T) {
		require.EqualValues(t, []redis.Z([]redis.Z{{Score: 1.79187397205302e+15, Member: "wtc one"}, {Score: 1.791875485187452e+15, Member: "union square"}, {Score: 1.791875761332224e+15, Member: "central park n/q/r"}, {Score: 1.791875796750882e+15, Member: "4545"}, {Score: 1.791875804419201e+15, Member: "lic market"}, {Score: 1.791875830079666e+15, Member: "q4"}, {Score: 1.791895905559723e+15, Member: "jfk"}}), rdb.ZRangeWithScores(ctx, "nyc", 0, -1).Val())
	})

	t.Run("GEORADIUS simple (sorted)", func(t *testing.T) {
		require.EqualValues(t, []redis.GeoLocation([]redis.GeoLocation{{Name: "central park n/q/r", Longitude: 0, Latitude: 0, Dist: 0, GeoHash: 0}, {Name: "4545", Longitude: 0, Latitude: 0, Dist: 0, GeoHash: 0}, {Name: "union square", Longitude: 0, Latitude: 0, Dist: 0, GeoHash: 0}}), rdb.GeoRadius(ctx, "nyc", -73.9798091, 40.7598464, &redis.GeoRadiusQuery{Radius: 3, Unit: "km", Sort: "asc"}).Val())
	})

	t.Run("GEORADIUS with COUNT", func(t *testing.T) {
		require.EqualValues(t, []redis.GeoLocation([]redis.GeoLocation{{Name: "central park n/q/r", Longitude: 0, Latitude: 0, Dist: 0, GeoHash: 0}, {Name: "4545", Longitude: 0, Latitude: 0, Dist: 0, GeoHash: 0}, {Name: "union square", Longitude: 0, Latitude: 0, Dist: 0, GeoHash: 0}}), rdb.GeoRadius(ctx, "nyc", -73.9798091, 40.7598464, &redis.GeoRadiusQuery{Radius: 10, Unit: "km", Sort: "asc", Count: 3}).Val())
	})

	t.Run("GEORADIUS HUGE, (redis issue #2767)", func(t *testing.T) {
		require.NoError(t, rdb.GeoAdd(ctx, "users", &redis.GeoLocation{Name: "user_000000", Longitude: -47.271613776683807, Latitude: -54.534504198047678}).Err())
		require.EqualValues(t, 1, len(rdb.GeoRadius(ctx, "users", 0, 0, &redis.GeoRadiusQuery{Radius: 50000, Unit: "km", WithCoord: true}).Val()))
	})

	t.Run("GEORADIUSBYMEMBER simple (sorted)", func(t *testing.T) {
		require.EqualValues(t, []redis.GeoLocation([]redis.GeoLocation{{Name: "wtc one", Longitude: 0, Latitude: 0, Dist: 0, GeoHash: 0}, {Name: "union square", Longitude: 0, Latitude: 0, Dist: 0, GeoHash: 0}, {Name: "central park n/q/r", Longitude: 0, Latitude: 0, Dist: 0, GeoHash: 0}, {Name: "4545", Longitude: 0, Latitude: 0, Dist: 0, GeoHash: 0}, {Name: "lic market", Longitude: 0, Latitude: 0, Dist: 0, GeoHash: 0}}), rdb.GeoRadiusByMember(ctx, "nyc", "wtc one", &redis.GeoRadiusQuery{Radius: 7, Unit: "km"}).Val())
	})

	t.Run("GEOHASH against non existing key", func(t *testing.T) {
		require.NoError(t, rdb.Del(ctx, "points").Err())
		require.EqualValues(t, []interface{}{nil, nil, nil}, rdb.Do(ctx, "GEOHASH", "points", "a", "b", "c").Val())
	})

	t.Run("GEOHASH is able to return geohash strings", func(t *testing.T) {
		require.NoError(t, rdb.Del(ctx, "points").Err())
		require.NoError(t, rdb.GeoAdd(ctx, "points", &redis.GeoLocation{Name: "test", Longitude: -5.6, Latitude: 42.6}).Err())
		require.EqualValues(t, []string([]string{"ezs42e44yx0"}), rdb.GeoHash(ctx, "points", "test").Val())
	})

	t.Run("GEOPOS against non existing key", func(t *testing.T) {
		require.NoError(t, rdb.Del(ctx, "points").Err())
		require.EqualValues(t, []interface{}{nil, nil, nil}, rdb.Do(ctx, "GEOPOS", "points", "a", "b", "c").Val())
	})

	t.Run("GEOPOS simple", func(t *testing.T) {
		require.NoError(t, rdb.Del(ctx, "points").Err())
		require.NoError(t, rdb.GeoAdd(ctx, "points", &redis.GeoLocation{Name: "a", Longitude: 10, Latitude: 20}, &redis.GeoLocation{Name: "b", Longitude: 30, Latitude: 40}).Err())
		cmd := rdb.GeoPos(ctx, "points", "a", "b")
		require.Less(t, math.Abs(cmd.Val()[0].Longitude-10), 0.001)
		require.Less(t, math.Abs(cmd.Val()[0].Latitude-20), 0.001)
		require.Less(t, math.Abs(cmd.Val()[1].Longitude-30), 0.001)
		require.Less(t, math.Abs(cmd.Val()[1].Latitude-40), 0.001)
	})

	t.Run("GEOPOS missing element", func(t *testing.T) {
		require.NoError(t, rdb.Del(ctx, "points").Err())
		require.NoError(t, rdb.GeoAdd(ctx, "points", &redis.GeoLocation{Name: "a", Longitude: 10, Latitude: 20}, &redis.GeoLocation{Name: "b", Longitude: 30, Latitude: 40}).Err())
		require.Nil(t, rdb.GeoPos(ctx, "points", "a", "x", "b").Val()[1])
	})

	t.Run("GEODIST simple & unit", func(t *testing.T) {
		require.NoError(t, rdb.Del(ctx, "points").Err())
		require.NoError(t, rdb.GeoAdd(ctx, "points", &redis.GeoLocation{Name: "Palermo", Longitude: 13.361389, Latitude: 38.115556}, &redis.GeoLocation{Name: "Catania", Longitude: 15.087269, Latitude: 37.502669}).Err())
		posVal := rdb.GeoDist(ctx, "points", "Palermo", "Catania", "m").Val()
		require.Greater(t, posVal, 166274.0)
		require.Less(t, posVal, 166275.0)
		distVal := rdb.GeoDist(ctx, "points", "Palermo", "Catania", "km").Val()
		require.Greater(t, distVal, 166.2)
		require.Less(t, distVal, 166.3)
	})

	t.Run("GEODIST missing elements", func(t *testing.T) {
		require.NoError(t, rdb.Del(ctx, "points").Err())
		require.NoError(t, rdb.GeoAdd(ctx, "points", &redis.GeoLocation{Name: "Palermo", Longitude: 13.361389, Latitude: 38.115556}, &redis.GeoLocation{Name: "Catania", Longitude: 15.087269, Latitude: 37.502669}).Err())
		require.EqualValues(t, 0, rdb.GeoDist(ctx, "points", "Palermo", "Agrigento", "").Val())
		require.EqualValues(t, 0, rdb.GeoDist(ctx, "points", "Ragusa", "Agrigento", "").Val())
		require.EqualValues(t, 0, rdb.GeoDist(ctx, "empty_key", "Palermo", "Catania", "").Val())
	})

	t.Run("GEORADIUS STORE option: syntax error", func(t *testing.T) {
		require.NoError(t, rdb.Del(ctx, "points").Err())
		require.NoError(t, rdb.GeoAdd(ctx, "points", &redis.GeoLocation{Name: "Palermo", Longitude: 13.361389, Latitude: 38.115556}, &redis.GeoLocation{Name: "Catania", Longitude: 15.087269, Latitude: 37.502669}).Err())
		require.ErrorContains(t, rdb.Do(ctx, "georadius", "points", 13.361389, 38.115556, 50, "km", "store").Err(), "syntax")
	})

	t.Run("GEORADIUS missing key", func(t *testing.T) {
		require.NoError(t, rdb.Del(ctx, "points").Err())
		require.EqualValues(t, []redis.GeoLocation([]redis.GeoLocation{}), rdb.GeoRadius(ctx, "points", 13.361389, 38.115556, &redis.GeoRadiusQuery{Radius: 50, Unit: "km"}).Val())
	})

	t.Run("GEORANGE STORE option: plain usage", func(t *testing.T) {
		require.NoError(t, rdb.Del(ctx, "points").Err())
		require.NoError(t, rdb.GeoAdd(ctx, "points", &redis.GeoLocation{Name: "Palermo", Longitude: 13.361389, Latitude: 38.115556}, &redis.GeoLocation{Name: "Catania", Longitude: 15.087269, Latitude: 37.502669}).Err())
		rdb.GeoRadiusStore(ctx, "points", 13.361389, 38.115556, &redis.GeoRadiusQuery{Radius: 500, Unit: "km", Store: "points2"})
		require.EqualValues(t, rdb.ZRange(ctx, "points", 0, -1).Val(), rdb.ZRange(ctx, "points2", 0, -1).Val())
	})

	type item struct {
		seed int64
		km   int64
		lon  float64
		lat  float64
	}
	regressionVectors := []item{
		{1482225976969, 7083, 81.634948934258375, 30.561509253718668},
		{1482340074151, 5416, -70.863281847379767, -46.347003465679947},
		{1499014685896, 6064, -89.818768962202014, -40.463868561416803},
		{1412, 156, 149.29737817929004, 15.95807862745508},
		{441574, 143, 59.235461856813856, 66.269555127373678},
		{160645, 187, -101.88575239939883, 49.061997951502917},
		{750269, 154, -90.187939661642517, 66.615930412251487},
		{342880, 145, 163.03472387745728, 64.012747720821181},
		{729955, 143, 137.86663517256579, 63.986745399416776},
		{939895, 151, 59.149620271823181, 65.204186651485145},
		{1412, 156, 149.29737817929004, 15.95807862745508},
		{564862, 149, 84.062063109158544, -65.685403922426232},
		{1546032440391, 16751, -1.8175081637769495, 20.665668878082954},
	}

	t.Run("GEOADD + GEORANGE randomized test", func(t *testing.T) {
		for attempt := 0; attempt < 30; attempt++ {
			var debuginfo string
			if attempt < len(regressionVectors) {
				rand.Seed(regressionVectors[attempt].seed)
				debuginfo += "rand seed is " + strconv.FormatInt(regressionVectors[attempt].seed, 10)
			} else {
				tmp := time.Now().UnixNano()
				rand.Seed(tmp)
				debuginfo += "rand seed is " + strconv.FormatInt(tmp, 10)
			}
			require.NoError(t, rdb.Del(ctx, "mypoints").Err())
			var radiusKm int64
			if util.RandomInt(10) == 0 {
				radiusKm = util.RandomInt(50000) + 10
			} else {
				radiusKm = util.RandomInt(200) + 10
			}
			if attempt < len(regressionVectors) {
				radiusKm = regressionVectors[attempt].km
			}
			radiusM := radiusKm * 1000
			searchLon, searchLat := geoRandomPoint()
			if attempt < len(regressionVectors) {
				searchLon = regressionVectors[attempt].lon
				searchLat = regressionVectors[attempt].lat
			}
			debuginfo += "Search area: " + strconv.FormatFloat(searchLon, 'f', 10, 64) + "," + strconv.FormatFloat(searchLat, 'f', 10, 64) + " " + strconv.FormatInt(radiusKm, 10) + " km"
			var result []string
			var argvs []*redis.GeoLocation
			for j := 0; j < 20000; j++ {
				lon, lat := geoRandomPoint()
				argvs = append(argvs, &redis.GeoLocation{Longitude: lon, Latitude: lat, Name: "place:" + strconv.Itoa(j)})
				distance := geoDistance(lon, lat, searchLon, searchLat)
				if distance < float64(radiusM) {
					result = append(result, "place:"+strconv.Itoa(j))
				}
				debuginfo += "place:" + strconv.FormatInt(int64(j), 10) + " " + strconv.FormatInt(int64(lon), 10) + " " + strconv.FormatInt(int64(lat), 10) + " " + strconv.FormatInt(int64(distance)/1000, 10) + " km"
			}
			require.NoError(t, rdb.GeoAdd(ctx, "mypoints", argvs...).Err())
			cmd := rdb.GeoRadius(ctx, "mypoints", searchLon, searchLat, &redis.GeoRadiusQuery{Radius: float64(radiusKm), Unit: "km"})
			sort.Strings(result)
			var res []string
			for _, i := range cmd.Val() {
				res = append(res, i.Name)
			}
			sort.Strings(res)
			equal := reflect.DeepEqual(res, result)
			testResult := true
			if !equal {
				roundingErrors := 0
				diff := compareLists(res, result)
				for _, i := range diff {
					cmd := rdb.GeoPos(ctx, "mypoints", i)
					mydist := geoDistance(cmd.Val()[0].Longitude, cmd.Val()[0].Latitude, searchLon, searchLat) / 1000
					if mydist/float64(radiusKm) > 0.999 {
						roundingErrors += 1
						continue
					}
					if mydist < float64(radiusM) {
						roundingErrors += 1
						continue
					}
				}
				if len(diff) == roundingErrors {
					equal = true
				}
			}
			if !equal {
				diff := compareLists(res, result)
				t.Log("Redis: ", res)
				t.Log("Gotest: ", result)
				t.Log("Diff: ", diff)
				t.Log("debuginfo: ", debuginfo)
				vis := make(map[string]int)
				for _, i := range result {
					vis[i] += 1
				}
				for _, i := range diff {
					var where string
					if _, ok := vis[i]; ok {
						where = "(only in Go test)"
					} else {
						where = "(only in Kvrocks)"
					}
					cmd := rdb.GeoPos(ctx, "mypoints", i)
					require.NoError(t, cmd.Err())
					mydis := geoDistance(cmd.Val()[0].Longitude, cmd.Val()[0].Latitude, searchLon, searchLat) / 1000
					t.Logf("%v -> %v %v %v", i, rdb.GeoPos(ctx, "mypoints", i).Val()[0], mydis, where)
				}
				testResult = false
			}
			if !testResult {
				require.FailNow(t, "not equal")
			}
		}
	})
}
