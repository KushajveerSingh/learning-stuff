from django.contrib import admin
from django.contrib.auth.admin import UserAdmin
from django.contrib.auth.models import Group

from .models import Promotion, Book, User


class BookAdmin(admin.ModelAdmin):
    list_display = ("isbn", "category", "title", "stock", "cost")


class UserAdminConfig(UserAdmin):
    list_display = ("email", "first_name", "last_name", "is_staff",
                    "is_active", "is_suspended")

    fieldsets = (
        ("Personal Info", {"fields": ("first_name", "last_name", "password",
                                      "phone")}),
        ("Address", {"fields": ("street", "city", "state", "zip_code",
                                "county", "country")}),
        ("Payment card 1", {"fields": ("card_name1",)}),
        ("Payment card 2", {"fields": ("card_name2",)}),
        ("Payment card 3", {"fields": ("card_name3",)}),
        ("Permissions", {"fields": ("is_staff", "is_active", "is_suspended")}),
        ("Promotions", {"fields": ("receive_promotions",)}),
    )

    add_fieldsets = (
        ("Personal Info", {"fields": ("first_name", "last_name", "email",
                                      "password", "phone")}),
        ("Address", {"fields": ("street", "city", "state", "zip_code",
                                "county", "country")}),
        ("Payment card 1", {"fields": ("card_name1", "card_num1", "card_exp1",
                                       "card_cvv1", "card_four1")}),
        ("Payment card 1", {"fields": ("card_name2", "card_num2", "card_exp2",
                                       "card_cvv2", "card_four3")}),
        ("Payment card 1", {"fields": ("card_name3", "card_num3", "card_exp3",
                                       "card_cvv3", "card_four3")}),
        ("Permissions", {"fields": ("is_staff", "is_active", "is_suspended")}),
        ("Promotions", {"fields": ("receive_promotions",)}),
    )

    ordering = ("email",)


class PromotionAdmin(admin.ModelAdmin):
    list_display = ("code", "percentage", "start_date", "end_date")

    def has_delete_permission(self, request, obj=None):
        return False

    def has_change_permission(self, request, obj=None):
        return False


admin.site.unregister(Group)
admin.site.register(Book, BookAdmin)
admin.site.register(User, UserAdminConfig)
admin.site.register(Promotion, PromotionAdmin)
