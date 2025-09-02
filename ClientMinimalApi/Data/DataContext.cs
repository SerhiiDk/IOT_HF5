using ClientMinimalApi.Data.Entities;
using Microsoft.EntityFrameworkCore;

namespace ClientMinimalApi.Data
{
    public class DataContext : DbContext
    {

        public DataContext(DbContextOptions<DataContext> options) :base(options)
        {
            
        }

        //protected override void OnConfiguring(DbContextOptionsBuilder optionsBuilder)
        //{
        //    optionsBuilder.UseMySql(
        //        "server=localhost;database=mydb;user=myuser;password=mypass;",
        //        new MySqlServerVersion(new Version(10, 6))
        //    );
        //}

        protected override void OnModelCreating(ModelBuilder modelBuilder)
        {
            modelBuilder.Entity<Devices>().ToTable("Devices");
            modelBuilder.Entity<Alarm>().ToTable("AlarmData");
        }
    }
}
